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

#include "reply.hpp"

#include "../../util/cast.hpp"

namespace http
{

const char ok_html[] = "";
const char bad_request_html[] = "{\"status\": 400,\"status_message\":\"Bad Request\"}";
const char internal_server_error_html[] =
    "{\"status\": 500,\"status_message\":\"Internal Server Error\"}";
const char seperators[] = {':', ' '};
const char crlf[] = {'\r', '\n'};
const std::string http_ok_string = "HTTP/1.0 200 OK\r\n";
const std::string http_bad_request_string = "HTTP/1.0 400 Bad Request\r\n";
const std::string http_internal_server_error_string = "HTTP/1.0 500 Internal Server Error\r\n";

void reply::set_size(const std::size_t size)
{
    for (header &h : headers)
    {
        if ("Content-Length" == h.name)
        {
            h.value = cast::integral_to_string(size);
        }
    }
}

void reply::set_uncompressed_size() { set_size(content.size()); }

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(status_to_buffer(status));
    for (const header &h : headers)
    {
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    buffers.push_back(boost::asio::buffer(content));
    return buffers;
}

std::vector<boost::asio::const_buffer> reply::headers_to_buffers()
{
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(status_to_buffer(status));
    for (const header &current_header : headers)
    {
        buffers.push_back(boost::asio::buffer(current_header.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(current_header.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    return buffers;
}

reply reply::stock_reply(const reply::status_type status)
{
    reply reply;
    reply.status = status;
    reply.content.clear();

    const std::string status_string = reply.status_to_string(status);
    reply.content.insert(reply.content.end(), status_string.begin(), status_string.end());
    reply.headers.emplace_back("Access-Control-Allow-Origin", "*");
    reply.headers.emplace_back("Content-Length", cast::integral_to_string(reply.content.size()));
    reply.headers.emplace_back("Content-Type", "text/html");
    return reply;
}

std::string reply::status_to_string(const reply::status_type status)
{
    if (reply::ok == status)
    {
        return ok_html;
    }
    if (reply::bad_request == status)
    {
        return bad_request_html;
    }
    return internal_server_error_html;
}

boost::asio::const_buffer reply::status_to_buffer(const reply::status_type status)
{
    if (reply::ok == status)
    {
        return boost::asio::buffer(http_ok_string);
    }
    if (reply::internal_server_error == status)
    {
        return boost::asio::buffer(http_internal_server_error_string);
    }
    return boost::asio::buffer(http_bad_request_string);
}

reply::reply() : status(ok) {}
}
