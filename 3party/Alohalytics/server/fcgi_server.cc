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

// clang-format off
/* This FastCGI server implementation is designed to store statistics received from remote clients.

Validity checks for requests should be mostly done on nginx side:
$request_method should be POST only
$content_length should be set and greater than zero (we re-check it below anyway)
$content_type should be set to application/alohalytics-binary-blob
$http_content_encoding should be set to gzip

This binary shoud be spawn as a FastCGI app, for example:
$ spawn-fcgi [-n] -p 8888 -- ./fcgi_server /dir/to/store/received/data [/optional/path/to/log.file]

# This is nginx config example to receive data from clients.
http {
  log_format alohalytics  '$remote_addr [$time_local] "$request" $status $content_length "$http_user_agent" $content_type $http_content_encoding';
  server {
    listen 8080;
    server_name localhost;
    # To hide nginx version.
    server_tokens off;

    location ~ ^/(ios|android)/(.+)/(.+) {
      set $OS $1;
      # Our clients send only POST requests.
      limit_except POST { deny all; }
      # Content-Length should be valid, but it is anyway checked on FastCGI app's code side.
      # Content-Type should be application/alohalytics-binary-blob
      if ($content_type != "application/alohalytics-binary-blob") {
        return 415; # Unsupported Media Type
      }
      if ($http_content_encoding != "gzip") {
        return 400; # Bad Request
      }
      client_body_buffer_size 1M;
      client_body_temp_path /tmp 2;
      client_max_body_size 100M;

      access_log /var/log/nginx/aloha-$OS-access.log alohalytics;
      # Unfortunately, error_log does not support variables.
      error_log  /var/log/nginx/aloha-error.log notice;

      fastcgi_pass_request_headers on;
      fastcgi_param REMOTE_ADDR $remote_addr;
      fastcgi_param REQUEST_URI $request_uri;
      fastcgi_pass 127.0.0.1:8888;
    }
  }
}
*/
// clang-format on

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <fcgiapp.h>
#include <fcgio.h>

#include "../src/file_manager.h"
#include "../src/logger.h"

#include "statistics_receiver.h"

using namespace std;

// Can be used as a basic check on the client-side if it has connected to the right server.
static const string kBodyTextInSuccessfulServerReply = "Mahalo";

// We need this global variable to reopen log file from SIGHUP handler.
struct CoutToFileRedirector;
CoutToFileRedirector * gRedirector = nullptr;
// Redirects all cout output into a file if good log_file_path was given in constructor.
// Can always ReopenLogFile() if needed (e.g. for log rotation).
struct CoutToFileRedirector {
  char const * path;
  unique_ptr<ofstream> log_file;
  streambuf * original_cout_buf;
  CoutToFileRedirector(const char * log_file_path) : path(log_file_path), original_cout_buf(cout.rdbuf()) {
    ReopenLogFile();
  }
  void ReopenLogFile() {
    if (path) {
      log_file.reset(nullptr);
      log_file.reset(new ofstream(path, ofstream::out | ofstream::app));
      if (log_file->good()) {
        cout.rdbuf(log_file->rdbuf());
        gRedirector = this;
      } else {
        ATLOG("ERROR: Can't open log file", path, "for writing.");
      }
    }
  }
  // Restore original cout streambuf.
  ~CoutToFileRedirector() { cout.rdbuf(original_cout_buf); }
};

int main(int argc, char * argv[]) {
  if (argc < 2) {
    ATLOG("Usage:", argv[0], "<directory to store received data> <path to error log file>");
    return -1;
  }

  // Redirect cout into a file if it was given in the command line.
  const CoutToFileRedirector log_redirector(argc > 2 ? argv[2] : nullptr);
  // Reopen log file on SIGHUP without server restart.
  ::signal(SIGHUP, [](int) { gRedirector->ReopenLogFile(); });

  int result = FCGX_Init();
  if (0 != result) {
    ATLOG("ERROR: FCGX_Init has failed with code", result);
    return result;
  }
  FCGX_Request request;
  result = FCGX_InitRequest(&request, 0, FCGI_FAIL_ACCEPT_ON_INTR);
  if (0 != result) {
    ATLOG("ERROR: FCGX_InitRequest has failed with code", result);
    return result;
  }
  alohalytics::StatisticsReceiver receiver(argv[1]);
  string gzipped_body;
  ATLOG("FastCGI Server instance is ready to serve clients' requests.");
  while (FCGX_Accept_r(&request) >= 0) {
    try {
      const char * content_length_str = FCGX_GetParam("HTTP_CONTENT_LENGTH", request.envp);
      long long content_length;
      if (!content_length_str || ((content_length = atoll(content_length_str)) <= 0)) {
        ATLOG("WARNING: Invalid or missing Content-Length header, request is ignored.");
        FCGX_FPrintF(request.out,
                     "Status: 411 Length Required\r\nContent-Type: text/plain\r\n\r\n411 Length Required\n");
        continue;
      }
      gzipped_body.resize(content_length);
      if (fcgi_istream(request.in).read(&gzipped_body[0], content_length).fail()) {
        ATLOG("WARNING: Can't read body contents, request is ignored.");
        FCGX_FPrintF(request.out, "Status: 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n400 Bad Request\n");
        continue;
      }

      const char * user_agent_str = FCGX_GetParam("HTTP_USER_AGENT", request.envp);
      if (!user_agent_str) {
        ATLOG("WARNING: Missing HTTP User-Agent.");
      }
      const char * request_uri_str = FCGX_GetParam("REQUEST_URI", request.envp);
      if (!request_uri_str) {
        ATLOG("WARNING: Missing REQUEST_URI.");
      }
      const char * remote_addr_str = FCGX_GetParam("REMOTE_ADDR", request.envp);
      if (!remote_addr_str) {
        ATLOG("WARNING: Missing REMOTE_ADDR.");
      }

      // Process and store received body.
      // This call can throw different exceptions.
      receiver.ProcessReceivedHTTPBody(gzipped_body, AlohalyticsBaseEvent::CurrentTimestamp(),
                                       remote_addr_str ? remote_addr_str : "", user_agent_str ? user_agent_str : "",
                                       request_uri_str ? request_uri_str : "");
      FCGX_FPrintF(request.out, "Status: 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s\n",
                   kBodyTextInSuccessfulServerReply.size(), kBodyTextInSuccessfulServerReply.c_str());
    } catch (const exception & ex) {
      ATLOG("ERROR: Exception while processing request: ", ex.what());
      FCGX_FPrintF(request.out,
                   "Status: 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n500 Internal Server Error\n");
      // TODO(AlexZ): Think about clients who can constantly fail because of bad data file.
      continue;
    }
  }
  ATLOG("Shutting down FastCGI server instance.");
  return 0;
}
