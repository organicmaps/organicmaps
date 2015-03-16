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

#include <array>
#include <fstream>
#include <iostream>  // std::cerr
#include <stdexcept> // std::runtime_error
#include <stdio.h>  // popen

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h> // close
#endif

// Used as a test stub for basic HTTP client implementation.
// Make sure that you have curl installed in the PATH.
// TODO(AlexZ): Not a production-ready implementation.
namespace alohalytics {

struct ScopedTmpFileDeleter {
  std::string file;
  ~ScopedTmpFileDeleter() {
    if (!file.empty()) {
      ::remove(file.c_str());
    }
  }
};

std::string RunCurl(const std::string& cmd) {
  FILE* pipe = ::popen(cmd.c_str(), "r");
  assert(pipe);
  std::array<char, 8 * 1024> s;
  std::string result;
  while (nullptr != ::fgets(s.data(), s.size(), pipe)) {
    result += s.data();
  }
  const int err = ::pclose(pipe);
  if (err) {
    throw std::runtime_error("Error " + std::to_string(err) + " while calling " + cmd);
  }
  return result;
}

bool HTTPClientPlatformWrapper::RunHTTPRequest() {
  // Last 3 chars in server's response will be http status code
  static constexpr size_t kCurlHttpCodeSize = 3;
  std::string cmd = "curl --max-redirs 0 -s -w '%{http_code}' ";
  if (!content_type_.empty()) {
    cmd += "-H 'Content-Type: application/json' ";
  }

  ScopedTmpFileDeleter deleter;
  if (!post_body_.empty()) {
    // POST body through tmp file to avoid breaking command line.
#ifdef _MSC_VER
    char tmp_file[L_tmpnam];
    ::tmpnam_s(tmp_file, L_tmpnam);
#else
    char tmp_file[] = "/tmp/alohalyticstmp-XXXXXX";
    int fd = ::mkstemp(tmp_file); // tmpnam is deprecated and insecure.
    if (fd < 0) {
      std::cerr << "Error: failed to create temporary file." << std::endl;
      return false;
    }
    ::close(fd);
#endif
    deleter.file = tmp_file;
    if (!(std::ofstream(deleter.file) << post_body_).good()) {
      std::cerr << "Error: failed to write into a temporary file." << std::endl;
      return false;
    }
    post_file_ = deleter.file;
  }
  if (!post_file_.empty()) {
    cmd += "--data-binary @" + post_file_ + " ";
  }

  cmd += url_requested_;
  try {
    server_response_ = RunCurl(cmd);
    error_code_ = -1;
    std::string & s = server_response_;
    if (s.size() < kCurlHttpCodeSize) {
      return false;
    }
    // Extract http status code from the last response line.
    error_code_ = std::stoi(s.substr(s.size() - kCurlHttpCodeSize));
    s.resize(s.size() - kCurlHttpCodeSize);

    if (!received_file_.empty()) {
      std::ofstream file(received_file_);
      file.exceptions(std::ios::failbit | std::ios::badbit);
      file << server_response_;
    }
  } catch (std::exception const & ex) {
    std::cerr << "Exception " << ex.what() << std::endl;
    return false;
  }
  // Should be safe here as we disabled redirects in curl's command line.
  url_received_ = url_requested_;
  return true;
}

}  // namespace aloha
