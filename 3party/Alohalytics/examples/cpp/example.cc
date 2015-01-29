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

// Core statistics engine.
#include "../../src/alohalytics.h"
// dflags is optional and is used here just for command line parameters parsing.
#include "dflags.h"

#include <iostream>
#include <thread>
#include <chrono>

DEFINE_string(server_url, "", "Statistics server url.");
DEFINE_string(event, "TestEvent", "Records given event.");
DEFINE_string(values,
              "",
              "Records event with single value (--values singleValue) or value pairs (--values "
              "key1=value1,key2=value2).");
DEFINE_string(storage,
              "",
              "Path to directory (with a slash at the end) to store recorded events before they are sent.");
DEFINE_bool(debug, true, "Enables debug mode for statistics engine.");
DEFINE_bool(upload, false, "If true, triggers event to forcebly upload all statistics to the server.");
DEFINE_double(sleep, 1, "The number of seconds to sleep before terminating.");
DEFINE_string(id, "0xDEADBABA", "Unique client id.");

using namespace std;
using alohalytics::Stats;

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  Stats& stats = Stats::Instance();

  if (FLAGS_debug) {
    stats.SetDebugMode(true);
  }

  if (!FLAGS_server_url.empty()) {
    stats.SetServerUrl(FLAGS_server_url);
  }

  if (!FLAGS_storage.empty()) {
    stats.SetStoragePath(FLAGS_storage);
  }

  if (!FLAGS_id.empty()) {
    stats.SetClientId(FLAGS_id);
  }

  if (!FLAGS_event.empty()) {
    if (!FLAGS_values.empty()) {
      string values = FLAGS_values;
      for (auto& c : values) {
        if (c == '=' || c == ',') {
          c = ' ';
        }
      }
      string key;
      alohalytics::TStringMap map;
      istringstream is(values);
      string it;
      while (is >> it) {
        if (key.empty()) {
          key = it;
          map[key] = "";
        } else {
          map[key] = it;
          key.clear();
        }
      }
      if (map.size() == 1 && map.begin()->second == "") {
        // Event with one value.
        stats.LogEvent(FLAGS_event, map.begin()->first);
      } else {
        // Event with many key=value pairs.
        stats.LogEvent(FLAGS_event, map);
      }
    } else {
      // Simple event.
      stats.LogEvent(FLAGS_event);
    }
  }

  if (FLAGS_upload) {
    stats.Upload();
  }

  this_thread::sleep_for(chrono::milliseconds(static_cast<uint32_t>(1e3 * FLAGS_sleep)));

  return 0;
}
