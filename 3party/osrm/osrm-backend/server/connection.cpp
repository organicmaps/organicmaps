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

#include "connection.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <string>
#include <vector>

namespace http
{

Connection::Connection(boost::asio::io_service &io_service, RequestHandler &handler)
    : strand(io_service), TCP_socket(io_service), request_handler(handler)
{
}

boost::asio::ip::tcp::socket &Connection::socket() { return TCP_socket; }

/// Start the first asynchronous operation for the connection.
void Connection::start()
{
    TCP_socket.async_read_some(
        boost::asio::buffer(incoming_data_buffer),
        strand.wrap(boost::bind(&Connection::handle_read, this->shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)));
}

void Connection::handle_read(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (error)
    {
        return;
    }

    // no error detected, let's parse the request
    compression_type compression_type(no_compression);
    osrm::tribool result;
    std::tie(result, compression_type) =
        request_parser.parse(current_request, incoming_data_buffer.data(),
                             incoming_data_buffer.data() + bytes_transferred);

    // the request has been parsed
    if (result == osrm::tribool::yes)
    {
        current_request.endpoint = TCP_socket.remote_endpoint().address();
        request_handler.handle_request(current_request, current_reply);

        // Header compression_header;
        std::vector<char> compressed_output;
        std::vector<boost::asio::const_buffer> output_buffer;

        // compress the result w/ gzip/deflate if requested
        switch (compression_type)
        {
        case deflate_rfc1951:
            // use deflate for compression
            current_reply.headers.insert(current_reply.headers.begin(),
                                         {"Content-Encoding", "deflate"});
            compressed_output = compress_buffers(current_reply.content, compression_type);
            current_reply.set_size(static_cast<unsigned>(compressed_output.size()));
            output_buffer = current_reply.headers_to_buffers();
            output_buffer.push_back(boost::asio::buffer(compressed_output));
            break;
        case gzip_rfc1952:
            // use gzip for compression
            current_reply.headers.insert(current_reply.headers.begin(),
                                         {"Content-Encoding", "gzip"});
            compressed_output = compress_buffers(current_reply.content, compression_type);
            current_reply.set_size(static_cast<unsigned>(compressed_output.size()));
            output_buffer = current_reply.headers_to_buffers();
            output_buffer.push_back(boost::asio::buffer(compressed_output));
            break;
        case no_compression:
            // don't use any compression
            current_reply.set_uncompressed_size();
            output_buffer = current_reply.to_buffers();
            break;
        }
        // write result to stream
        boost::asio::async_write(
            TCP_socket, output_buffer,
            strand.wrap(boost::bind(&Connection::handle_write, this->shared_from_this(),
                                    boost::asio::placeholders::error)));
    }
    else if (result == osrm::tribool::no)
    { // request is not parseable
        current_reply = reply::stock_reply(reply::bad_request);

        boost::asio::async_write(
            TCP_socket, current_reply.to_buffers(),
            strand.wrap(boost::bind(&Connection::handle_write, this->shared_from_this(),
                                    boost::asio::placeholders::error)));
    }
    else
    {
        // we don't have a result yet, so continue reading
        TCP_socket.async_read_some(
            boost::asio::buffer(incoming_data_buffer),
            strand.wrap(boost::bind(&Connection::handle_read, this->shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred)));
    }
}

/// Handle completion of a write operation.
void Connection::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
        // Initiate graceful connection closure.
        boost::system::error_code ignore_error;
        TCP_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_error);
    }
}

std::vector<char> Connection::compress_buffers(const std::vector<char> &uncompressed_data,
                                               const compression_type compression_type)
{
    boost::iostreams::gzip_params compression_parameters;

    // there's a trade-off between speed and size. speed wins
    compression_parameters.level = boost::iostreams::zlib::best_speed;
    // check which compression flavor is used
    if (deflate_rfc1951 == compression_type)
    {
        compression_parameters.noheader = true;
    }

    std::vector<char> compressed_data;
    // plug data into boost's compression stream
    boost::iostreams::filtering_ostream gzip_stream;
    gzip_stream.push(boost::iostreams::gzip_compressor(compression_parameters));
    gzip_stream.push(boost::iostreams::back_inserter(compressed_data));
    gzip_stream.write(&uncompressed_data[0], uncompressed_data.size());
    boost::iostreams::close(gzip_stream);

    return compressed_data;
}
}
