/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef REPLY_H
#define REPLY_H

#include "Header.h"

#include <boost/asio.hpp>

#include <vector>

namespace http
{

const char okHTML[] = "";
const char badRequestHTML[] = "{\"status\": 400,\"status_message\":\"Bad Request\"}";
const char internalServerErrorHTML[] =
    "{\"status\": 500,\"status_message\":\"Internal Server Error\"}";
const char seperators[] = {':', ' '};
const char crlf[] = {'\r', '\n'};
const std::string okString = "HTTP/1.0 200 OK\r\n";
const std::string badRequestString = "HTTP/1.0 400 Bad Request\r\n";
const std::string internalServerErrorString = "HTTP/1.0 500 Internal Server Error\r\n";

class Reply
{
  public:
    enum status_type
    { ok = 200,
      badRequest = 400,
      internalServerError = 500 } status;

    std::vector<Header> headers;
    std::vector<boost::asio::const_buffer> ToBuffers();
    std::vector<boost::asio::const_buffer> HeaderstoBuffers();
    std::vector<char> content;
    static Reply StockReply(status_type status);
    void SetSize(const unsigned size);
    void SetUncompressedSize();

    Reply();

  private:
    std::string ToString(Reply::status_type status);
    boost::asio::const_buffer ToBuffer(Reply::status_type status);
};
}

#endif // REPLY_H
