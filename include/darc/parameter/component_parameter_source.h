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
 * DARC ComponentParameterSource class
 *
 * \author Morten Kjaergaard
 */

#pragma once

#include <darc/parameter/parameter_source_abstract.h>
#include <darc/parameter/status.h>

namespace darc
{
namespace parameter
{

class ComponentParameterSource
{
public:

protected:
  typedef std::map<const std::string, ParameterSourceAbstractPtr> ParameterSourceListType;
  ParameterSourceListType parameter_list_;

  std::string component_name_;

public:
  ComponentParameterSource() :
    component_name_(component_name)
  {
  }

  template<typename T>
  void addParameterSource(boost::shared_ptr<ParameterSource<T> > parameter_source)
  {
    parameter_list_[parameter_source->getName()] = parameter_source;
  }

  void update()
  {
    for( ParameterSourceListType::iterator it = parameter_list_.begin(); it != parameter_list_.end(); it++ )
    {
      (*it)->update();
    }
  }

};

}
}