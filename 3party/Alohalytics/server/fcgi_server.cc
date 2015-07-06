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

#include "../src/logger.h"

#include "statistics_receiver.h"

using namespace std;

// Can be used as a basic check on the client-side if it has connected to the right server.
static const string kBodyTextForGoodServerReply = "Mahalo";
static const string kBodyTextForBadServerReply = "Hohono";

// We always reply to our clients that we have received everything they sent, even if it was a complete junk.
// The difference is only in the body of the reply.
void Reply200OKWithBody(FCGX_Stream * out, const string & body) {
  FCGX_FPrintF(out, "Status: 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s\n", body.size(),
               body.c_str());
}

// Global variables to correctly reopen data and log files after signals from logrotate utility.
volatile sig_atomic_t gReceivedSIGHUP = 0;
volatile sig_atomic_t gReceivedSIGUSR1 = 0;
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
    if (!path) {
      return;
    }
    log_file.reset(nullptr);
    log_file.reset(new ofstream(path, ofstream::out | ofstream::app));
    if (log_file->good()) {
      cout.rdbuf(log_file->rdbuf());
    } else {
      ATLOG("ERROR: Can't open log file", path, "for writing.");
    }
  }
  // Restore original cout streambuf.
  ~CoutToFileRedirector() { cout.rdbuf(original_cout_buf); }
};

int main(int argc, char * argv[]) {
  if (argc < 2) {
    ALOG("Usage:", argv[0], "<directory to store received data> <path to error log file>");
    ALOG("  - SIGHUP reopens main data file and SIGUSR1 reopens debug log file for logrotate utility.");
    return -1;
  }

  // Redirect cout into a file if it was given in the command line.
  CoutToFileRedirector log_redirector(argc > 2 ? argv[2] : nullptr);
  // Correctly reopen data file on SIGHUP for logrotate.
  if (SIG_ERR == ::signal(SIGHUP, [](int) { gReceivedSIGHUP = SIGHUP; })) {
    ATLOG("WARNING: Can't set SIGHUP handler. Logrotate will not work correctly.");
  }
  // Correctly reopen debug log file on SIGUSR1 for logrotate.
  if (SIG_ERR == ::signal(SIGUSR1, [](int) { gReceivedSIGUSR1 = SIGUSR1; })) {
    ATLOG("WARNING: Can't set SIGUSR1 handler. Logrotate will not work correctly.");
  }

  int result = FCGX_Init();
  if (0 != result) {
    ALOG("ERROR: FCGX_Init has failed with code", result);
    return result;
  }
  FCGX_Request request;
  result = FCGX_InitRequest(&request, 0, FCGI_FAIL_ACCEPT_ON_INTR);
  if (0 != result) {
    ALOG("ERROR: FCGX_InitRequest has failed with code", result);
    return result;
  }
  alohalytics::StatisticsReceiver receiver(argv[1]);
  string gzipped_body;
  long long content_length;
  const char * remote_addr_str;
  const char * request_uri_str;
  const char * user_agent_str;
  ATLOG("FastCGI Server instance is ready to serve clients' requests.");
  while (FCGX_Accept_r(&request) >= 0) {
    // Correctly reopen data file in the queue.
    if (gReceivedSIGHUP == SIGHUP) {
      receiver.ReopenDataFile();
      gReceivedSIGHUP = 0;
    }
    // Correctly reopen debug log file.
    if (gReceivedSIGUSR1 == SIGUSR1) {
      log_redirector.ReopenLogFile();
      gReceivedSIGUSR1 = 0;
    }
    FCGX_FPrintF(request.out, "Status: 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s\n",
                 kBodyTextForGoodServerReply.size(), kBodyTextForGoodServerReply.c_str());
    try {
      remote_addr_str = FCGX_GetParam("REMOTE_ADDR", request.envp);
      if (!remote_addr_str) {
        ATLOG("WARNING: Missing REMOTE_ADDR. Please check your http server configuration.");
        remote_addr_str = "";
      }
      request_uri_str = FCGX_GetParam("REQUEST_URI", request.envp);
      if (!request_uri_str) {
        ATLOG("WARNING: Missing REQUEST_URI. Please check your http server configuration.");
        request_uri_str = "";
      }
      user_agent_str = FCGX_GetParam("HTTP_USER_AGENT", request.envp);
      if (!user_agent_str) {
        ATLOG("WARNING: Missing HTTP User-Agent. Please check your http server configuration.");
        user_agent_str = "";
      }

      const char * content_length_str = FCGX_GetParam("HTTP_CONTENT_LENGTH", request.envp);
      content_length = 0;
      if (!content_length_str || ((content_length = atoll(content_length_str)) <= 0)) {
        ATLOG("WARNING: Request is ignored due to invalid or missing Content-Length header", content_length_str,
              remote_addr_str, request_uri_str, user_agent_str);
        Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
        continue;
      }
      // TODO(AlexZ): Should we make a better check for Content-Length or basic exception handling would be enough?
      gzipped_body.resize(content_length);
      if (fcgi_istream(request.in).read(&gzipped_body[0], content_length).fail()) {
        ATLOG("WARNING: Request is ignored because it's body can't be read.", remote_addr_str, request_uri_str,
              user_agent_str);
        Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
        continue;
      }

      // Process and store received body.
      // This call can throw different exceptions.
      receiver.ProcessReceivedHTTPBody(gzipped_body, AlohalyticsBaseEvent::CurrentTimestamp(), remote_addr_str,
                                       user_agent_str, request_uri_str);
      Reply200OKWithBody(request.out, kBodyTextForGoodServerReply);
    } catch (const exception & ex) {
      ATLOG("ERROR: Exception was thrown:", ex.what(), remote_addr_str, request_uri_str, user_agent_str);
      // TODO(AlexZ): Log "bad" received body for investigation.
      Reply200OKWithBody(request.out, kBodyTextForBadServerReply);
    }
  }
  ATLOG("Shutting down FastCGI server instance.");
  return 0;
}
