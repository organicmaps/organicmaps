/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef REPLY_HPP
#define REPLY_HPP

#include "header.hpp"

#include <boost/asio.hpp>

#include <vector>

namespace http
{
class reply
{
  public:
    enum status_type
    {
        ok = 200,
        bad_request = 400,
        internal_server_error = 500
    } status;

    std::vector<header> headers;
    std::vector<boost::asio::const_buffer> to_buffers();
    std::vector<boost::asio::const_buffer> headers_to_buffers();
    std::vector<char> content;
    static reply stock_reply(const status_type status);
    void set_size(const std::size_t size);
    void set_uncompressed_size();

    reply();

  private:
    std::string status_to_string(reply::status_type status);
    boost::asio::const_buffer status_to_buffer(reply::status_type status);
};
}

#endif // REPLY_HPP
