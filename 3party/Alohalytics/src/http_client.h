/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

namespace alohalytics {

class HTTPClientPlatformWrapper {
 public:
  enum {
    kNotInitialized = -1,
  };

 private:
  std::string url_requested_;
  // Contains final content's url taking redirects (if any) into an account.
  std::string url_received_;
  int error_code_ = kNotInitialized;
  std::string body_file_;
  // Used instead of server_reply_ if set.
  std::string received_file_;
  // Data we received from the server if output_file_ wasn't initialized.
  std::string server_response_;
  std::string content_type_;
  std::string content_type_received_;
  std::string content_encoding_;
  std::string content_encoding_received_;
  std::string user_agent_;
  std::string body_data_;
  std::string http_method_ = "GET";
  std::string basic_auth_user_;
  std::string basic_auth_password_;
  bool debug_mode_ = false;

  HTTPClientPlatformWrapper(const HTTPClientPlatformWrapper&) = delete;
  HTTPClientPlatformWrapper(HTTPClientPlatformWrapper&&) = delete;
  HTTPClientPlatformWrapper& operator=(const HTTPClientPlatformWrapper&) = delete;

 public:
  HTTPClientPlatformWrapper() = default;
  HTTPClientPlatformWrapper(const std::string& url) : url_requested_(url) {}
  HTTPClientPlatformWrapper& set_debug_mode(bool debug_mode) {
    debug_mode_ = debug_mode;
    return *this;
  }
  HTTPClientPlatformWrapper& set_url_requested(const std::string& url) {
    url_requested_ = url;
    return *this;
  }
  HTTPClientPlatformWrapper& set_http_method(const std::string& method) {
    http_method_ = method;
    return *this;
  }
  // This method is mutually exclusive with set_body_data().
  HTTPClientPlatformWrapper& set_body_file(const std::string& body_file,
                                           const std::string& content_type,
                                           const std::string& http_method = "POST",
                                           const std::string& content_encoding = "") {
    body_file_ = body_file;
    body_data_.clear();
    content_type_ = content_type;
    http_method_ = http_method;
    content_encoding_ = content_encoding;
    return *this;
  }
  // If set, stores server reply in file specified.
  HTTPClientPlatformWrapper& set_received_file(const std::string& received_file) {
    received_file_ = received_file;
    return *this;
  }
  HTTPClientPlatformWrapper& set_user_agent(const std::string& user_agent) {
    user_agent_ = user_agent;
    return *this;
  }
  // This method is mutually exclusive with set_body_file().
  HTTPClientPlatformWrapper& set_body_data(const std::string& body_data,
                                           const std::string& content_type,
                                           const std::string& http_method = "POST",
                                           const std::string& content_encoding = "") {
    body_data_ = body_data;
    body_file_.clear();
    content_type_ = content_type;
    http_method_ = http_method;
    content_encoding_ = content_encoding;
    return *this;
  }
  // Move version to avoid string copying.
  // This method is mutually exclusive with set_body_file().
  HTTPClientPlatformWrapper& set_body_data(std::string&& body_data,
                                           const std::string& content_type,
                                           const std::string& http_method = "POST",
                                           const std::string& content_encoding = "") {
    body_data_ = std::move(body_data);
    body_file_.clear();
    content_type_ = content_type;
    http_method_ = http_method;
    content_encoding_ = content_encoding;
    return *this;
  }
  // HTTP Basic Auth.
  HTTPClientPlatformWrapper& set_user_and_password(const std::string& user, const std::string& password) {
    basic_auth_user_ = user;
    basic_auth_password_ = password;
    return *this;
  }

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true if connection was made and server returned something (200, 404, etc.).
  // @note Implementations should transparently support all needed HTTP redirects
  bool RunHTTPRequest();

  std::string const& url_requested() const { return url_requested_; }
  // @returns empty string in the case of error
  std::string const& url_received() const { return url_received_; }
  bool was_redirected() const { return url_requested_ != url_received_; }
  // Mix of HTTP errors (in case of successful connection) and system-dependent error codes,
  // in the simplest success case use 'if (200 == client.error_code())' // 200 means OK in HTTP
  int error_code() const { return error_code_; }
  std::string const& server_response() const { return server_response_; }

};  // class HTTPClientPlatformWrapper

}  // namespace alohalytics

#endif  // HTTP_CLIENT_H
