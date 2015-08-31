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

// Helper for easier events processing.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../../src/event_base.h"

#include <chrono>
#include <iostream>
#include <set>
#include <memory>
#include <string>

namespace alohalytics {

typedef std::function<void(const AlohalyticsIdServerEvent * ie, const AlohalyticsKeyEvent * e)> TLambda;

// Reads and processes all cereal events from stdin.
struct Processor {
  uint64_t total_events_processed = 0;
  // Used to get total unique users count.
  std::set<std::string> unique_user_ids;
  std::chrono::seconds::rep processing_time_sec;

  Processor(TLambda lambda) {
    cereal::BinaryInputArchive ar(std::cin);
    std::unique_ptr<AlohalyticsBaseEvent> ptr, server_id_ptr;
    const auto start_time = std::chrono::system_clock::now();
    while (true) {
      try {
        ar(ptr);
      } catch (const cereal::Exception & ex) {
        if (std::string("Failed to read 4 bytes from input stream! Read 0") != ex.what()) {
          // The exception above is a normal one, Cereal lacks to detect the end of the stream.
          std::cerr << ex.what() << std::endl;
        }
        break;
      }
      const AlohalyticsIdServerEvent * id_event = dynamic_cast<const AlohalyticsIdServerEvent *>(ptr.get());
      if (id_event) {
        unique_user_ids.insert(id_event->id);
        server_id_ptr = std::move(ptr);
      } else {
        lambda(static_cast<const AlohalyticsIdServerEvent *>(server_id_ptr.get()),
               static_cast<const AlohalyticsKeyEvent *>(ptr.get()));
        // Do not count id events as they are created automatically.
        ++total_events_processed;
      }
    }
    processing_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time).count();
  }

// Use cerr to avoid mixing with cout.
void PrintStatistics() {
  std::cerr << "Processing time: " << processing_time_sec << " seconds." << std::endl;
  std::cerr << "Unique users: " << unique_user_ids.size() << std::endl;
  std::cerr << "Total events processed: " << total_events_processed << std::endl;
}

}; // class Processor

} // namespace alohalytics
