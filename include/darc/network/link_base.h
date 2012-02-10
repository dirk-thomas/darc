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
 * DARC LinkBase class
 *
 * \author Morten Kjaergaard
 */

#ifndef __DARC_NETWORK_LINK_BASE_H_INCLUDED__
#define __DARC_NETWORK_LINK_BASE_H_INCLUDED__

#include <boost/shared_ptr.hpp>
#include <darc/shared_buffer.h>
#include <darc/network/packet/header.h>
#include <darc/network/link_manager_callback_if.h>

namespace darc
{
namespace network
{

class LinkBase
{
protected:
  LinkManagerCallbackIF * callback_;

  LinkBase(LinkManagerCallbackIF * callback) :
    callback_(callback)
  {
  }

  ~LinkBase()
  {
  }

public:
  virtual void sendPacket(const ID& outbound_id,
			  packet::Header::PayloadType type,
			  SharedBuffer buffer, std::size_t data_len) = 0;
  virtual void sendDiscoverToAll() = 0;
  virtual void sendDiscoverReply(const ID& remote_outbound_id) = 0;

};

typedef boost::shared_ptr<LinkBase> LinkBasePtr;

}
}

#endif
