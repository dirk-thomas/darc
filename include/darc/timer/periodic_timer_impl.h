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
 * DARC TimerImpl class
 *
 * \author Morten Kjaergaard
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <darc/owner.h>
#include <darc/timer/periodic_timer_ctrl_handle.h>

namespace darc
{
namespace timer
{

class PeriodicTimerImpl : public boost::asio::deadline_timer, public boost::enable_shared_from_this<PeriodicTimerImpl>
{
public:
  typedef boost::function<void()> CallbackType;

protected:
  typedef boost::shared_ptr<PeriodicTimerImpl> Ptr;

  CallbackType callback_;

  boost::posix_time::time_duration period_;
  boost::posix_time::ptime expected_deadline_;

protected:
  PeriodicTimerImpl(boost::asio::io_service * io_service, CallbackType callback, boost::posix_time::time_duration period) :
    boost::asio::deadline_timer(*io_service, period),
    callback_(callback),
    period_(period)
  {
    expected_deadline_ = boost::posix_time::microsec_clock::universal_time() + period;
    async_wait( boost::bind( &PeriodicTimerImpl::handler, this ) );
  }

  void handler()// const boost::system::error_code& error )
  {
    boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::universal_time() - expected_deadline_;
    expected_deadline_ += period_;
    //    std::cout << diff.total_milliseconds() << std::endl;
    expires_from_now( period_ - diff );

    async_wait( boost::bind( &PeriodicTimerImpl::handler, this ) );

    //    Consumer::cpu_usage_.start();
    callback_();
    //    Consumer::cpu_usage_.stop();
  }

public:
  static PeriodicTimerImpl::Ptr create(boost::asio::io_service * io_service, CallbackType callback, boost::posix_time::time_duration period)
  {
    return PeriodicTimerImpl::Ptr( new PeriodicTimerImpl(io_service, callback, period) );
  }

  PeriodicTimerCtrlHandle createCtrlHandle()
  {
    return PeriodicTimerCtrlHandle(shared_from_this());
  }

};

typedef boost::shared_ptr<PeriodicTimerImpl> PeriodicTimerImplPtr;

}
}
