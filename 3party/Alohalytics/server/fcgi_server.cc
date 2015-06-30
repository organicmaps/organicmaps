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

// This FastCGI server implementation is designed to store statistics received from remote clients.

// Validity checks for requests should be mostly done on nginx side:
// $request_method should be POST only
// $content_length should be set and greater than zero (we re-check it below anyway)
// $content_type should be set to application/alohalytics-binary-blob
// $http_content_encoding should be set to gzip

#include <chrono>

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

// TODO(AlexZ): Pass log file as a command line parameter and use it for logging when running as daemon.
int main(int argc, char * argv[]) {
  if (argc < 2) {
    ALOG("Usage:", argv[0], "<directory to store received data>");
    return -1;
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
  ALOG("FastCGI Server instance is ready to serve clients' requests.");
  while (FCGX_Accept_r(&request) >= 0) {
    try {
      const char * content_length_str = FCGX_GetParam("HTTP_CONTENT_LENGTH", request.envp);
      long long content_length;
      if (!content_length_str || ((content_length = atoll(content_length_str)) <= 0)) {
        ALOG("WARNING: Invalid or missing Content-Length header, request is ignored.");
        FCGX_FPrintF(request.out,
                     "Status: 411 Length Required\r\nContent-Type: text/plain\r\n\r\n411 Length Required\n");
        continue;
      }
      gzipped_body.resize(content_length);
      if (fcgi_istream(request.in).read(&gzipped_body[0], content_length).fail()) {
        ALOG("WARNING: Can't read body contents, request is ignored.");
        FCGX_FPrintF(request.out, "Status: 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n400 Bad Request\n");
        continue;
      }

      const char * user_agent_str = FCGX_GetParam("HTTP_USER_AGENT", request.envp);
      if (user_agent_str) {
      } else {
        ALOG("WARNING: Missing HTTP User-Agent.");
      }
      const char * request_uri_str = FCGX_GetParam("REQUEST_URI", request.envp);
      if (request_uri_str) {
      } else {
        ALOG("WARNING: Missing REQUEST_URI.");
      }
      const char * remote_addr_str = FCGX_GetParam("REMOTE_ADDR", request.envp);
      if (remote_addr_str) {
      } else {
        ALOG("WARNING: Missing REMOTE_ADDR.");
      }

      // Process and store received body.
      // This call can throw different exceptions.
      receiver.ProcessReceivedHTTPBody(gzipped_body, AlohalyticsBaseEvent::CurrentTimestamp(),
                                       remote_addr_str ? remote_addr_str : "", user_agent_str ? user_agent_str : "",
                                       request_uri_str ? request_uri_str : "");
      FCGX_FPrintF(request.out, "Status: 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s\n",
                   kBodyTextInSuccessfulServerReply.size(), kBodyTextInSuccessfulServerReply.c_str());

    } catch (const std::exception & ex) {
      ALOG("ERROR: Exception while processing request: ", ex.what());
      FCGX_FPrintF(request.out,
                   "Status: 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n500 Internal Server Error\n");
      // TODO(AlexZ): Full log, possibly into the file, to get more details about the error. Also think about clients
      // who can constantly fail because of bad data file.
      continue;
    }
  }
  ALOG("Shutting down FastCGI server instance.");
  return 0;
}
