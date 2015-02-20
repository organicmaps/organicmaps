/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

#include "../http_client.h"

#include <stdio.h>  // popen
#include <fstream>
#include <iostream>  // std::cerr

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

// Used as a test stub for basic HTTP client implementation.

namespace alohalytics {

std::string RunCurl(const std::string& cmd) {
  FILE* pipe = ::popen(cmd.c_str(), "r");
  assert(pipe);
  char s[8 * 1024];
  ::fgets(s, sizeof(s) / sizeof(s[0]), pipe);
  const int err = ::pclose(pipe);
  if (err) {
    std::cerr << "Error " << err << " while calling " << cmd << std::endl;
  }
  return s;
}

// Not fully implemented.
bool HTTPClientPlatformWrapper::RunHTTPRequest() {
  // Last 3 chars in server's response will be http status code
  std::string cmd = "curl -s -w '%{http_code}' ";
  if (!content_type_.empty()) {
    cmd += "-H 'Content-Type: application/json' ";
  }
  if (!post_body_.empty()) {
    // POST body through tmp file to avoid breaking command line.
    char tmp_file[L_tmpnam];
#ifdef _MSC_VER
    ::tmpnam_s(tmp_file, L_tmpnam);
#else
    ::tmpnam(tmp_file);
#endif
    std::ofstream(tmp_file) << post_body_;
    post_file_ = tmp_file;
  }
  if (!post_file_.empty()) {
    cmd += "--data-binary @" + post_file_ + " ";
  }

  cmd += url_requested_;
  server_response_ = RunCurl(cmd);

  // Clean up tmp file if any was created.
  if (!post_body_.empty() && !post_file_.empty()) {
    ::remove(post_file_.c_str());
  }

  // TODO(AlexZ): Detect if we did not make any connection and return false.
  // Extract http status code from the last response line.
  error_code_ = std::stoi(server_response_.substr(server_response_.size() - 3));
  server_response_.resize(server_response_.size() - 4);

  if (!received_file_.empty()) {
    std::ofstream(received_file_) << server_response_;
  }

  return true;
}

}  // namespace aloha
