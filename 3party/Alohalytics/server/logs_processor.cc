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
/*******************************************************************************
 DISCLAIMER: This code was created as a quick and simple work-around until fully functional server will be ready.

 It processes nginx logs from stdin and extracts temporarily stored files into statistics data files.
 Log entry example:
 192.168.3.123 [18/Jun/2015:03:43:01 +0300] "POST /2 HTTP/1.1" 200 107 /path_to_files_saved_by_nginx/393/167/188/0188167393 "Dalvik/1.6.0 (Linux; U; Android 4.4.4; m1 note Build/KTU84P)" application/alohalytics-binary-blob gzip

 Such log entries are created by the following nginx configuration:
 log_format storebodyinfile '$remote_addr [$time_local] "$request" $status $content_length $request_body_file "$http_user_agent" $content_type $http_content_encoding';
 server {
   <server information here>
   location = /2 {
     limit_except POST { deny all; }
     client_body_temp_path /path_to_files_saved_by_nginx 3 3 3;
     client_body_in_file_only on;
     client_body_buffer_size 1M;
     client_max_body_size 100M;
     access_log access.log storebodyinfile;
     error_log  error.log warn;
     proxy_pass_request_headers off;
     proxy_set_body off;
     proxy_redirect off;
     proxy_pass http://localhost/ok;
   }
   location /ok {
     access_log off;
     add_header Content-Type text/plain;
     return 200 Ok;
   }
 }
********************************************************************************/
// clang-format on

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../src/event_base.h"
#include "../src/gzip_wrapper.h"
#include "../src/file_manager.h"

#include "statistics_receiver.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <time.h>  // non-C++, POSIX only strptime

using namespace std;
using namespace alohalytics;

static void DeleteFile(const string & file) { std::remove(file.c_str()); }

int main(int argc, char * argv[]) {
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <directory to store merged file>" << endl;
    return -1;
  }
  string directory(argv[1]);
  FileManager::AppendDirectorySlash(directory);
  if (!FileManager::IsDirectoryWritable(directory)) {
    cout << "ERROR: Directory " << directory << " is not writable, please specify another one." << endl;
    return -1;
  }

  // Parse nginx log entries from stdin one by one.
  string log_entry;
  size_t good_files_processed = 0, corrupted_files_removed = 0, other_files_removed = 0;
  size_t files_total_size = 0;
  StatisticsReceiver receiver(directory);
  while (getline(cin, log_entry)) {
    // IP address.
    string::size_type start_pos = 0;
    string::size_type end_pos = log_entry.find_first_of(' ');
    if (end_pos == string::npos) {
      cout << "WARNING: Can't get IP address. Invalid log entry? " << log_entry << endl;
      continue;
    }
    const string ip(log_entry, start_pos, end_pos - start_pos);
    // Basic IP validity check.
    if (count(ip.begin(), ip.end(), '.') != 3) {
      cout << "WARNING: Invalid IP address: " << ip << endl;
      continue;
    }

    // Server timestamp.
    start_pos = end_pos + 1;
    end_pos = log_entry.find_first_of(' ', start_pos);
    if (end_pos == string::npos) {
      cout << "WARNING: Can't get server timestamp. Invalid log entry? " << log_entry << endl;
      continue;
    }
    struct tm stm;
    ::memset(&stm, 0, sizeof(stm));
    if (NULL == ::strptime(&log_entry[start_pos], "[%d/%b/%Y:%H:%M:%S", &stm)) {
      cout << "WARNING: Can't parse server timestamp: " << log_entry.substr(start_pos, end_pos - start_pos) << endl;
      continue;
    }
    // TODO(AlexZ): Do not rely on time_t equal to seconds from epoch.
    const uint64_t server_timestamp_ms_from_epoch = mktime(&stm) * 1000;

    // Request URI.
    start_pos = log_entry.find_first_of('/', end_pos);
    if (start_pos == string::npos) {
      cout << "WARNING: Can't get request uri. Invalid log entry? " << log_entry << endl;
      continue;
    }
    end_pos = log_entry.find_first_of(' ', start_pos);
    if (end_pos == string::npos) {
      cout << "WARNING: Can't get request uri. Invalid log entry? " << log_entry << endl;
      continue;
    }
    const string uri(log_entry, start_pos, end_pos - start_pos);

    // HTTP Code should be 200 for correct data.
    start_pos = log_entry.find_first_of(' ', end_pos + 1);
    int http_code;
    if ((istringstream(log_entry.substr(start_pos + 1, 3)) >> http_code).fail()) {
      cout << "WARNING: can't parse HTTP code. Invalid log entry? " << log_entry << endl;
      continue;
    }

    if (http_code != 200 && http_code != 499) {
      cout << "Ignoring non-successful HTTP response in the log: " << http_code << " " << log_entry << endl;
      continue;
    }

    // Path to the file with a POST body.
    start_pos = log_entry.find_first_of('/', start_pos);
    if (start_pos == string::npos) {
      cout << "WARNING: Can't get path to file. Invalid log entry? " << log_entry << endl;
      continue;
    }
    end_pos = log_entry.find_first_of(' ', start_pos);
    if (end_pos == string::npos) {
      cout << "WARNING: Can't get path to file. Invalid log entry? " << log_entry << endl;
      continue;
    }
    const string file_path(log_entry, start_pos, end_pos - start_pos);

    // Now we have a path to the file and can safely delete it.
    // nginx HTTP code 499 means that file was received but client has aborted connection before
    // receiving anything from the server.
    // This data will be sent by client again, so we can safely delete these files.
    if (http_code == 499) {
      DeleteFile(file_path);
      ++other_files_removed;
      continue;
    }

    // HTTP User-Agent.
    start_pos = log_entry.find_first_of('"', end_pos);
    if (start_pos == string::npos) {
      cout << "WARNING: Can't get User-Agent. Invalid log entry? " << log_entry << endl;
      continue;
    }
    end_pos = log_entry.find("\" ", start_pos + 1);
    if (end_pos == string::npos) {
      cout << "WARNING: Can't get User-Agent. Invalid log entry? " << log_entry << endl;
      continue;
    }
    const string user_agent(log_entry, start_pos + 1, end_pos - start_pos - 1);

    // Check that Content-Type and Content-Encoding are correct.
    if (string::npos == log_entry.find(" application/alohalytics-binary-blob gzip", end_pos + 1)) {
      cout << "WARNING: Content-Type and Content-Encoding are incorrect. Invalid log entry? " << log_entry << endl;
      continue;
    }

    const string gzipped_body = FileManager::ReadFileAsString(file_path);
    files_total_size += gzipped_body.size();
    if (gzipped_body.empty()) {
      cout << "WARNING: Can't load contents of " << file_path << ". Log entry: " << log_entry << endl;
      continue;
    }

    try {
      receiver.ProcessReceivedHTTPBody(gzipped_body, server_timestamp_ms_from_epoch, ip, user_agent, uri);
    } catch (const std::exception & ex) {
      cout << "WARNING: Corrupted file " << file_path << ", exception: " << ex.what() << endl;
      DeleteFile(file_path);
      ++corrupted_files_removed;
      continue;
    }
    DeleteFile(file_path);
    ++good_files_processed;
  }
  cout << "Successfully processed " << good_files_processed << " files." << endl;
  cout << "Deleted " << corrupted_files_removed << " corrupted and " << other_files_removed << " files." << endl;
  cout << "Good and corrupted files total size: " << files_total_size << endl;

  return 0;
}
