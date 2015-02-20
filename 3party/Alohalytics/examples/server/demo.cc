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

#include "../../src/Bricks/rtti/dispatcher.h"

#include <iostream>
#include <iomanip>
#include <typeinfo>

// If you use only virtual functions in your events, you don't need any Processor at all.
// Here we process some events with Processor for example only.
struct Processor {
  void operator()(const AlohalyticsBaseEvent &event) {
    std::cout << "Unhandled event of type " << typeid(event).name() << " with timestamp " << event.ToString()
              << std::endl;
  }
  void operator()(const AlohalyticsIdEvent &event) { std::cout << event.ToString() << std::endl; }
  void operator()(const AlohalyticsKeyEvent &event) { std::cout << event.ToString() << std::endl; }
  void operator()(const AlohalyticsKeyValueEvent &event) { std::cout << event.ToString() << std::endl; }
  void operator()(const AlohalyticsKeyPairsEvent &event) { std::cout << event.ToString() << std::endl; }
  void operator()(const AlohalyticsKeyPairsLocationEvent &event) { std::cout << event.ToString() << std::endl; }
};

int main(int, char **) {
  cereal::BinaryInputArchive ar(std::cin);
  Processor processor;
  try {
    while (std::cin.good()) {
      std::unique_ptr<AlohalyticsBaseEvent> ptr;
      ar(ptr);
      bricks::rtti::RuntimeDispatcher<AlohalyticsBaseEvent, AlohalyticsKeyPairsLocationEvent,
                                      AlohalyticsKeyPairsEvent, AlohalyticsIdEvent, AlohalyticsKeyValueEvent,
                                      AlohalyticsKeyEvent>::DispatchCall(*ptr, processor);
    }
  } catch (const cereal::Exception &) {
  }
  return 0;
}
