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

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "http/compression_type.hpp"
#include "http/reply.hpp"
#include "http/request.hpp"
#include "request_parser.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/version.hpp>

#include <memory>
#include <vector>

// workaround for incomplete std::shared_ptr compatibility in old boost versions
#if BOOST_VERSION < 105300 || defined BOOST_NO_CXX11_SMART_PTR

namespace boost
{
template <class T> const T *get_pointer(std::shared_ptr<T> const &p) { return p.get(); }

template <class T> T *get_pointer(std::shared_ptr<T> &p) { return p.get(); }
} // namespace boost

#endif

class RequestHandler;

namespace http
{

/// Represents a single connection from a client.
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(boost::asio::io_service &io_service, RequestHandler &handler);
    Connection(const Connection &) = delete;
    Connection() = delete;

    boost::asio::ip::tcp::socket &socket();

    /// Start the first asynchronous operation for the connection.
    void start();

  private:
    void handle_read(const boost::system::error_code &e, std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code &e);

    std::vector<char> compress_buffers(const std::vector<char> &uncompressed_data,
                                       const compression_type compression_type);

    boost::asio::io_service::strand strand;
    boost::asio::ip::tcp::socket TCP_socket;
    RequestHandler &request_handler;
    RequestParser request_parser;
    boost::array<char, 8192> incoming_data_buffer;
    request current_request;
    reply current_reply;
};

} // namespace http

#endif // CONNECTION_HPP
