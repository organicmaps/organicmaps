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

// Small demo which prints raw (ungzipped) cereal stream from stdin.

// This define is needed to preserve client's timestamps in events.
#define ALOHALYTICS_SERVER
#include "../../src/event_base.h"

#include "processor.h"

#include <iostream>

int main(int, char **) {
  cereal::BinaryInputArchive ar(std::cin);
  const AlohalyticsIdServerEvent * previous = nullptr;
  alohalytics::Processor([&previous](const AlohalyticsIdServerEvent * se, const AlohalyticsKeyEvent * e) {
    if (previous != se) {
      std::cout << se->ToString() << std::endl;
      previous = se;
    }
    std::cout << e->ToString() << std::endl;
  }).PrintStatistics();
  return 0;
}
