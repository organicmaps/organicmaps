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

// This tool splits events by days when they were received on a server.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../src/event_base.h"
#include "../src/file_manager.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <utility>

using namespace std;

string GenerateFileNameForTimestamp(uint64_t ms_from_epoch) {
  time_t stamp = static_cast<time_t>(ms_from_epoch / 1000);
  // Add one day to fit logrotate's logic: it always names files one day ahead for daily rotations.
  // Don't forget to keep it in the head for calculations...
  stamp += 60 * 60 * 24;
  char buf[100] = "";
  strftime(buf, sizeof(buf), "-%Y%m%d", ::gmtime(&stamp));
  return string("alohalytics_messages") + buf;
}

int main(int argc, char ** argv) {
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <output_folder>" << endl;
    return -1;
  }
  string directory = argv[1];
  alohalytics::FileManager::AppendDirectorySlash(directory);
  if (!alohalytics::FileManager::IsDirectoryWritable(directory)) {
    cout << "Error: directory " << directory << " is not writable." << endl;
    return -1;
  }

  map<string, unique_ptr<ofstream>> files;
  cereal::BinaryInputArchive ar(cin);
  try {
    unique_ptr<AlohalyticsBaseEvent> ptr;
    ofstream * out_file = nullptr;
    while (cin) {
      ar(ptr);
      const AlohalyticsIdServerEvent * se = dynamic_cast<const AlohalyticsIdServerEvent *>(ptr.get());
      if (se) {
        const string path = directory + GenerateFileNameForTimestamp(se->server_timestamp);
        const auto found = files.find(path);
        if (found == files.end()) {
          out_file = new ofstream(path, ios_base::app | ios_base::out | ios_base::binary);
          unique_ptr<ofstream> fptr(out_file);
          fptr->exceptions(ifstream::failbit | ifstream::badbit);
          files.emplace(path, std::move(fptr));
        } else {
          out_file = found->second.get();
        }
      }
      std::ostringstream sstream;
      { cereal::BinaryOutputArchive(sstream) << ptr; }
      const string binary_event = sstream.str();
      out_file->write(&binary_event[0], binary_event.size());
    }
  } catch (const cereal::Exception & ex) {
    cerr << ex.what() << endl;
  }
  return 0;
}
