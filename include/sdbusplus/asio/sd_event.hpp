/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include <systemd/sd-event.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/error_code.hpp>

namespace sdbusplus
{

namespace asio
{
/* A simple class to integrate the sd_event_loop into the boost::asio io_context
 * in case a boost::asio user needs sd_events
 */
class sd_event_wrapper
{
  public:
    sd_event_wrapper(boost::asio::io_context& io) :
        evt(nullptr), descriptor(io), io(io)
    {
        sd_event_default(&evt);
        if (evt)
        {
            descriptor.assign(sd_event_get_fd(evt));
            async_run();
        }
    }
    sd_event_wrapper(sd_event* evt, boost::asio::io_context& io) :
        evt(evt), descriptor(io), io(io)
    {
        if (evt)
        {
            sd_event_ref(evt);
            descriptor.assign(sd_event_get_fd(evt));
            async_run();
        }
    }
    ~sd_event_wrapper()
    {
        // sd_event really wants to close the descriptor on its own
        // so this class must merely release it
        descriptor.release();
        sd_event_unref(evt);
    }
    // process one event step in the queue
    // return true if the queue is still alive
    void run()
    {
        int ret;
        int state = sd_event_get_state(evt);
        switch (state)
        {
            case SD_EVENT_INITIAL:
                ret = sd_event_prepare(evt);
                if (ret > 0)
                {
                    async_run();
                }
                else if (ret == 0)
                {
                    async_wait();
                }
                break;
            case SD_EVENT_ARMED:
                ret = sd_event_wait(evt, 0);
                if (ret >= 0)
                {
                    async_run();
                }
                break;
            case SD_EVENT_PENDING:
                ret = sd_event_dispatch(evt);
                if (ret > 0)
                {
                    async_run();
                }
                break;
            case SD_EVENT_FINISHED:
                break;
            default:
                // throw something?
                // doing nothing will break out of the async loop
                break;
        }
    }
    sd_event* get() const
    {
        return evt;
    }

  private:
    void async_run()
    {
        boost::asio::post(io, [this]() { run(); });
    }
    void async_wait()
    {
        descriptor.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                              [this](const boost::system::error_code& error) {
                                  if (!error)
                                  {
                                      run();
                                  }
                              });
    }
    sd_event* evt;
    boost::asio::posix::stream_descriptor descriptor;
    boost::asio::io_context& io;
};

} // namespace asio

} // namespace sdbusplus
