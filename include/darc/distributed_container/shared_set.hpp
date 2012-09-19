/*
 * Copyright (c) 2012, Prevas A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Prevas A/S nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *
 *
 * \author Morten Kjaergaard
 */

#include <map>
#include <boost/shared_ptr.hpp>

#include <darc/id.hpp>

#include <darc/distributed_container/container_base.hpp>
#include <darc/distributed_container/container_manager.hpp>
#include <darc/distributed_container/control_packet.hpp>
#include <darc/distributed_container/header_packet.hpp>
#include <darc/distributed_container/control_packet.hpp>
#include <darc/distributed_container/update_packet.hpp>

#include <darc/serializer/boost.hpp>

namespace darc
{
namespace distributed_container
{

template<typename Key, typename T>
class connection
{
protected:

  typedef std::pair<ID/*origin*/, T> entry_type;
  typedef std::pair<Key, entry_type> transfer_type;

  ID instance_id_;
  ID remote_location_id_;
  ID remote_instance_id_;
  container_manager * manager_;

  uint32_t last_sent_index_;
  uint32_t last_received_index_;

  typedef std::map<Key, entry_type> list_type;
  list_type list_;

public:
  connection(container_manager * manager,
        const ID& instance_id,
	     const ID& remote_location_id,
	     const ID& remote_instance_id) :
	  instance_id_(instance_id),
    remote_location_id_(remote_location_id),
    remote_instance_id_(remote_instance_id),
    manager_(manager),
    last_sent_index_(0),
    last_received_index_(0)
  {
  }

  void do_connect()
  {
    control_packet ctrl;
    ctrl.command = control_packet::connect;
    outbound_data<serializer::boost_serializer, control_packet> o_ctrl(ctrl);
    manager_->send_to_location(
          instance_id_,
          remote_location_id_,
		     remote_instance_id_,
		     header_packet::control,
		     o_ctrl);
  }

  void increment(const ID& informer,
		 const Key& key,
		 const ID& origin,
		 const T& value,
		 uint32_t state_index)
  {
    entry_type entry(origin, value);

    last_sent_index_ = state_index;
    if(informer != remote_instance_id_)
    {
      update_packet update;
      update.start_index = state_index;
      update.end_index = state_index;
      update.type = update_packet::partial;
      update.num_entries = 1;

      outbound_data<serializer::boost_serializer, update_packet> o_update(update);
      transfer_type item(key, entry);

      outbound_data<serializer::boost_serializer, transfer_type> o_item(item);

      outbound_pair o_data(o_update, o_item);

      manager_->send_to_location(instance_id_,
          remote_location_id_,
		       remote_instance_id_,
		       header_packet::update,
		       o_data);
    }
  }

  void handle_update(const header_packet& header, const update_packet& update, buffer::shared_buffer data)
  {
    assert(update.num_entries == 1);
    inbound_data<serializer::boost_serializer, transfer_type> i_item(data);
    list_.insert(list_type::value_type(
		   i_item.get().first,
		   i_item.get().second));
  }

};


template<typename Key, typename T>
class shared_set : public container_base
{
protected:
  typedef connection<Key, T> connection_type;
  typedef boost::shared_ptr<connection_type> connection_ptr;

  typedef std::map</*informer_instance*/ID, connection_ptr> connection_list_type;
  connection_list_type connection_list_;

  struct entry_type
  {
    ID origin;
    ID informer;
    T value;

    entry_type(const ID& origin, const ID& informer, const T& value) :
      origin(origin),
      informer(informer),
      value(value)
  };

  typedef std::map<Key, entry_type> entry_list_type;

  entry_list_type entry_list_;
  ID instance_id_;
  uint32_t state_index_;

  shared_set() :
    instance_id_(ID::create()),
    state_index_(0)
  {
  }

  void insert(const Key& key, const T& value)
  {
    entry_type entry(instance_id_, instance_id, T);
    entry_list_.insert(
      entry_list_type::value_type(Key, entry));
    state_index_++;

    // do it in flush instead
    for(connection_list_type::iterator it = connection_list_.begin();
	it != connection_list_.end();
	it++)
    {               /*informer*/
      it->increment(instance_id, key, entry.origin, entry.value, state_index_);
    }
  }

  //void remote_insert

  void connect(const ID& remote_location_id,
	       const ID& remote_instance_id)
  {
    assert(connection_list_.find(remote_instance_id == connection_list_.end()));
    connection_ptr c = boost::make_shared<connection>(
      manager_,
      remote_location_id,
      remote_instance_id);
    connection_list_.insert(
      connection_list_type::value_type(remote_instance_id, c));
    c->do_connect();
  }

  void recv(const ID& src_location_id, const header_packet& hdr, darc::buffer::shared_buffer data)
  {
    switch(hdr.payload_type)
    {
    case header_packet::control:
    {
      inbound_data<serializer::boost_serializer, control_packet> i_control(data);
      handle_ctrl(src_location_id, hdr, i_control.get());
    }
    break;
    case header_packet::update:
    {
      inbound_data<serializer::boost_serializer, update_packet> i_update(data);
      handle_uodate(src_location_id, hdr, i_update.get(), data);
    }
    break;
    default:
      assert(false);
    }
  }

  void handle_ctrl(const ID& src_location_id, const header_packet& header, const control_packet& ctrl)
  {
    assert(ctrl.command == control_packet::connect);
    assert(connection_list_.find(header.src_instance_id) != connection_list_.end());

    connection_ptr c = boost::make_shared<connection>(
      manager_,
      src_location_id,
      header.src_instance_id);
    connection_list_.insert(
      connection_list_type::value_type(remote_instance_id, c));
  }

  void handle_update(const ID& src_location_id,
		     const header_packet& header,
		     const update_packet& update,
		     darc::buffer::shared_buffer data)
  {
    connection_list_type::iterator item = connection_list_.find(src_location_id);
    assert(item != connection_list_.end());
    item->handle_update(header, update, data);
  }

};

}
}
