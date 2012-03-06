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
 * DARC RemoteDispatcherManager class
 *
 * \author Morten Kjaergaard
 */

#ifndef __DARC_PUBLISH_REMOTE_DISPATCH_HANDLER_H_INCLUDED__
#define __DARC_PUBLISH_REMOTE_DISPATCH_HANDLER_H_INCLUDED__

#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <darc/serialization.h>
#include <darc/network/packet/header.h>
#include <darc/network/packet/message.h>
#include <darc/network/packet/message_subscribe.h>
#include <darc/network/packet/message_publish_info.h>
#include <darc/log.h>

namespace darc
{
namespace pubsub
{

class RemoteDispatcher
{
private:
  struct SubscribedTopicInfo
  {
    std::string topic;
    std::string type_name;

    bool operator<(const SubscribedTopicInfo& other) const
    {
      return topic < other.topic;
    }

    SubscribedTopicInfo(const std::string& topic, const std::string& type_name) :
      topic(topic),
      type_name(type_name)
    {
    }
  };

private:
  boost::asio::io_service * io_service_;

  // Function to dispatch locally
  typedef boost::function<void (const std::string& topic, SharedBuffer)> LocalDispatchFunctionType;
  LocalDispatchFunctionType local_dispatch_function_;

  // Function to send to remote node
  typedef boost::function<void (network::packet::Header::PayloadType, const ID&, SharedBuffer, std::size_t )> SendToNodeFunctionType;
  SendToNodeFunctionType send_to_node_function_;

  // Topics we subscribe to
  typedef std::set<SubscribedTopicInfo> SubscribedTopicsType;
  SubscribedTopicsType subscribed_topics_;

  // Topics we publish (todo: tmp hack, not really required by the msg system, should be handled differently)
  typedef std::set<SubscribedTopicInfo> PublishedTopicsType;
  PublishedTopicsType published_topics_;

  // Topics other subscribe to
  typedef std::multimap<std::string, ID> RemoteSubscribersType;
  typedef std::pair<RemoteSubscribersType::iterator, RemoteSubscribersType::iterator> RemoteSubscribersRangeType;
  RemoteSubscribersType remote_subscribers_;

  // Topics other publishes to (todo: tmp hack, also handle differnetly)
  typedef std::multimap<std::string, ID> RemotePublishersType;
  typedef std::pair<RemotePublishersType::iterator, RemotePublishersType::iterator> RemotePublishersRangeType;
  RemotePublishersType remote_publishers_;

  // Signals
  boost::signal<void (const std::string&, const std::string&, size_t)> signal_remote_subscriber_change_;
  boost::signal<void (const std::string&, const std::string&, size_t)> signal_remote_publisher_change_;

public:
  RemoteDispatcher( boost::asio::io_service * io_service ) :
    io_service_( io_service )
  {
  }

  boost::signal<void (const std::string&, const std::string&, size_t)>& remoteSubscriberChangeSignal()
  {
    return signal_remote_subscriber_change_;
  }

  boost::signal<void (const std::string&, const std::string&, size_t)>& remotePublisherChangeSignal()
  {
    return signal_remote_publisher_change_;
  }

  void messageReceiveHandler( const network::packet::Header& header, SharedBuffer buffer, std::size_t data_len )
  {
    // Parse Msg Packet
    network::packet::Message msg_packet;
    size_t msg_header_size = msg_packet.read( buffer.data(), data_len );
    buffer.addOffset( msg_header_size );

    // Dispatch to local subscribers
    local_dispatch_function_( msg_packet.topic, buffer );
  }

  void subscriptionReceiveHandler( const network::packet::Header& header, SharedBuffer buffer, std::size_t data_len )
  {
    // Parse Packet
    network::packet::MessageSubscribe packet;
    packet.read( buffer.data(), data_len );

    // Todo check the correct type
    DARC_INFO("-- Subscription for topic: %s %s %s", packet.topic.c_str(), packet.type_name.c_str(), header.sender_node_id.short_string().c_str());
    remote_subscribers_.insert(RemoteSubscribersType::value_type(packet.topic, header.sender_node_id));

    // Trigger Signal
    signal_remote_subscriber_change_(packet.topic, packet.type_name, remote_subscribers_.count(packet.topic));
  }

  void publishInfoReceiveHandler( const network::packet::Header& header, SharedBuffer buffer, std::size_t data_len )
  {
    // Parse Packet
    network::packet::MessagePublishInfo packet;
    packet.read( buffer.data(), data_len );

    // Todo check the correct type
    DARC_INFO("-- Publish Info for topic: %s %s %s", packet.topic.c_str(), packet.type_name.c_str(), header.sender_node_id.short_string().c_str());
    remote_publishers_.insert(RemoteSubscribersType::value_type(packet.topic, header.sender_node_id));

    // Trigger Signal
    signal_remote_publisher_change_(packet.topic, packet.type_name, remote_publishers_.count(packet.topic));
  }

  void setLocalDispatchFunction( LocalDispatchFunctionType local_dispatch_function )
  {
    local_dispatch_function_ = local_dispatch_function;
  }

  void newRemoteNodeHandler(const ID& remote_node_id)
  {
    for(SubscribedTopicsType::iterator it = subscribed_topics_.begin();
	it != subscribed_topics_.end();
	it++)
    {
      sendSubscription(it->topic, it->type_name, remote_node_id);
    }
    for(SubscribedTopicsType::iterator it = published_topics_.begin();
	it != published_topics_.end();
	it++)
    {
      sendPublish(it->topic, it->type_name, remote_node_id);
    }
  }

  void registerSubscription(const std::string& topic, const std::string& type_name)
  {
    // check that type is correct
    SubscribedTopicInfo item(topic, type_name);
    subscribed_topics_.insert(item);
    sendSubscription(topic, type_name, ID::null());
  }

  void registerPublisher(const std::string& topic, const std::string& type_name)
  {
    // check that type is correct
    SubscribedTopicInfo item(topic, type_name);
    published_topics_.insert(item);
    sendPublish(topic, type_name, ID::null());
  }

  void sendSubscription(const std::string& topic, const std::string& type_name, const ID& remote_node_id)
  {
    DARC_INFO("Sending subscription to %s for topic: %s", remote_node_id.short_string().c_str(), topic.c_str());

    // Allocate buffer. todo: derive required size?
    std::size_t data_len = 1024;
    SharedBuffer buffer = SharedBuffer::create(data_len);

    // Message Subscription Packet
    network::packet::MessageSubscribe packet;
    packet.topic = topic;
    packet.type_name = type_name;
    packet.write(buffer.data(), buffer.size());

    assert( send_to_node_function_ );
    send_to_node_function_( network::packet::Header::MSG_SUBSCRIBE, remote_node_id, buffer, data_len );
  }

  void sendPublish(const std::string& topic, const std::string& type_name, const ID& remote_node_id)
  {
    DARC_INFO("Sending publish info to %s for topic: %s", remote_node_id.short_string().c_str(), topic.c_str());

    // Allocate buffer. todo: derive required size?
    std::size_t data_len = 1024;
    SharedBuffer buffer = SharedBuffer::create(data_len);

    // Message Subscription Packet
    network::packet::MessagePublishInfo packet;
    packet.topic = topic;
    packet.type_name = type_name;
    packet.write(buffer.data(), buffer.size());

    assert( send_to_node_function_ );
    send_to_node_function_( network::packet::Header::MSG_PUBLISH_INFO, remote_node_id, buffer, data_len );
  }

  void setSendToNodeFunction( SendToNodeFunctionType send_to_node_function )
  {
    send_to_node_function_ = send_to_node_function;
  }

  // Triggered by asio post
  template<typename T>
  void serializeAndDispatch( const std::string topic, const boost::shared_ptr<const T> msg )
  {
    // Allocate buffer. todo: derive required size from msg
    std::size_t data_len = 1024*32;
    SharedBuffer buffer = SharedBuffer::create(data_len);

    // Message Header
    network::packet::Message msg_header(topic);
    std::size_t pos = msg_header.write( buffer.data(), buffer.size() );

    // todo: Put common serialization stuff somewhere to reuse
    // Write Type Info
    pos += network::packet::Parser::writeString( ros::message_traits::DataType<T>::value(), buffer.data() + pos, buffer.size() - pos );
    // MD5
    pos += network::packet::Parser::writeUint64( ros::message_traits::MD5Sum<T>::static_value1, buffer.data() + pos, buffer.size() - pos );
    pos += network::packet::Parser::writeUint64( ros::message_traits::MD5Sum<T>::static_value2, buffer.data() + pos, buffer.size() - pos );

    // Serialize actual message
    ros::serialization::OStream ostream( buffer.data() + pos, buffer.size() - pos );
    ros::serialization::serialize( ostream, *(msg.get()) );

    assert( send_to_node_function_ );

    // Find the subscribers
    RemoteSubscribersRangeType subscribers = remote_subscribers_.equal_range(topic);
    for(RemoteSubscribersType::iterator it = subscribers.first; it != subscribers.second; it++)
    {
      send_to_node_function_( network::packet::Header::MSG_PACKET, it->second, buffer, data_len );
    }
  }

  // Called by LocalDispatcher
  template<typename T>
  void postRemoteDispatch( const std::string& topic, const boost::shared_ptr<const T> msg )
  {
    if(remote_subscribers_.count(topic) != 0)
    {
      io_service_->post( boost::bind(&RemoteDispatcher::serializeAndDispatch<T>, this, topic, msg) );
    }
  }

};

typedef boost::shared_ptr<RemoteDispatcher> RemoteDispatcherPtr;

}
}

#endif
