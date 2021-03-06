/*
 * Copyright (c) 2011, Prevas A/S
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
 * DARC InboundLink class
 *
 * \author Morten Kjaergaard
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <darc/peer/peer.hpp>
#include <darc/buffer/shared_buffer.hpp>
#include <darc/network/link_header_packet.hpp>
#include <darc/network/discover_packet.hpp>
#include <darc/network/discover_reply_packet.hpp>
#include <darc/network/outbound_data.hpp>
#include <darc/network/inbound_data.hpp>
#include <darc/serializer/boost.hpp>
#include <darc/buffer/const_size_buffer.hpp>
#include <darc/id_arg.hpp>
#include <iris/static_scope.hpp>

namespace darc
{
namespace network
{

class network_manager;

class inbound_link_base : public virtual iris::static_scope<iris::Info>
{
protected:
  peer& peer_;
  class network_manager * manager_;

protected:
  inbound_link_base(class network_manager * manager, peer& p) :
    manager_(manager),
    peer_(p)
  {
  }

  virtual ~inbound_link_base()
  {
  }

public:
  virtual void send_packet(const ID& outbound_id,
                          const ID& dest_peer_id,
                          const uint16_t packet_type,
                          buffer::shared_buffer data) = 0;

  virtual void send_packet_to_all(const ID& dest_peer_id,
                                  const uint16_t packet_type,
                                  buffer::shared_buffer data) = 0;

  class network_manager* network_manager()
  {
    return manager_;
  }

  void packet_received(buffer::shared_buffer header_data,
                       buffer::shared_buffer body_data);

  void handle_discover_packet(const ID& src_peer_id, buffer::shared_buffer& data)
  {
    inbound_data<darc::serializer::boost_serializer, discover_packet> dp_i(data);

    slog<iris::Debug>("Received DISCOVER",
                      "peer_id", iris::arg<ID>(src_peer_id),
                      "remote outbound_id", iris::arg<ID>(dp_i.get().outbound_id));

    discover_reply_packet drp;
    drp.outbound_id = dp_i.get().outbound_id;
    outbound_data<darc::serializer::boost_serializer, discover_reply_packet> o_drp(drp);

    buffer::shared_buffer buffer = boost::make_shared<buffer::const_size_buffer>(1024); // todo

    o_drp.pack(buffer);

    send_packet_to_all(src_peer_id, link_header_packet::DISCOVER_REPLY, buffer);
  }

  void handle_discover_reply_packet(const ID& src_peer_id, buffer::shared_buffer& data);

  void sendDiscover(const ID& outbound_id)
  {
    slog<iris::Debug>("Sending DISCOVER",
                      "Outbound ID", iris::arg<ID>(outbound_id));

    discover_packet dp;
    dp.outbound_id = outbound_id;
    outbound_data<darc::serializer::boost_serializer, discover_packet> o_dp(dp);

    buffer::shared_buffer buffer = boost::make_shared<buffer::const_size_buffer>(1024); // todo

    o_dp.pack(buffer);

    send_packet(outbound_id, ID::null(), link_header_packet::DISCOVER, buffer);
  }

/*
  void sendDiscoverReply(const ID& remote_outbound_id, const ID& remote_node_id)
  {
  // Create packet
  std::size_t data_len = 1024*32;
  SharedBuffer buffer = SharedBufferArray::create(data_len);
  network::packet::DiscoverReply discover_reply(remote_outbound_id);
  std::size_t len = discover_reply.write( buffer.data(), buffer.size() );

  // Send packet
  sendPacketToAll(network::packet::Header::DISCOVER_REPLY_PACKET, remote_node_id, buffer, len );
  }
*/
};

typedef boost::shared_ptr<inbound_link_base> inbound_link_ptr;

}
}
