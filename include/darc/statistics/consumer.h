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
 * DARC Consumer class
 *
 * \author Morten Kjaergaard
 */

#pragma once

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <darc/statistics/cpu_usage.h>

namespace darc
{
namespace statistics
{

class Consumer
{
protected:
  typedef boost::shared_ptr<Consumer> Ptr;

  typedef std::map<const std::string, Consumer::Ptr> ConsumerListType;
  ConsumerListType child_consumers_;

  CPUUsage cpu_usage_;

public:
  Consumer()
  {
  }

  void addChild(const std::string name, Consumer::Ptr child_consumer)
  {
    // todo: check for name clash
    child_consumers_[name] = child_consumer;
  }

  Consumer::Ptr getChild(const std::string name)
  {
    // todo: check for existance
    return child_consumers_[name];
  }

};

typedef boost::shared_ptr<Consumer> ConsumerPtr;

}
}
