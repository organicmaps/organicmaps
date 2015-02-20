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

#ifndef EVENT_BASE_H
#define EVENT_BASE_H

// Define ALOHALYTICS_SERVER when using this header on a server-side.

#include <chrono>
#include <sstream>

#include "cereal/include/cereal.hpp"
#include "cereal/include/types/base_class.hpp"
#include "cereal/include/types/polymorphic.hpp"
#include "cereal/include/archives/binary.hpp"
#include "cereal/include/types/string.hpp"
#include "cereal/include/types/map.hpp"

#include "location.h"

// For easier processing on a server side, every statistics event should derive from this base class.
struct AlohalyticsBaseEvent {
  uint64_t timestamp;

  virtual std::string ToString() const {
    const time_t timet = static_cast<const time_t>(timestamp / 1000);
    char buf[100];
    if (::strftime(buf, 100, "%e-%b-%Y %H:%M:%S", ::gmtime(&timet))) {
      return buf;
    } else {
      return "INVALID_TIME";
    }
  }

  static uint64_t CurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
  }

// To avoid unnecessary calls on a server side
#ifndef ALOHALYTICS_SERVER
  AlohalyticsBaseEvent() : timestamp(CurrentTimestamp()) {}
#endif

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(timestamp));
  }

  // To use polymorphism on a server side.
  virtual ~AlohalyticsBaseEvent() = default;
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsBaseEvent, "b")

// Special event in the beginning of each block (file) sent to stats server.
// Designed to mark all events in this data block as belonging to one user with specified id.
struct AlohalyticsIdEvent : public AlohalyticsBaseEvent {
  std::string id;

  virtual std::string ToString() const { return AlohalyticsBaseEvent::ToString() + " ID: " + id; }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
    ar(CEREAL_NVP(id));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsIdEvent, "i")

// Simple event with a string name (key) only.
struct AlohalyticsKeyEvent : public AlohalyticsBaseEvent {
  std::string key;

  virtual std::string ToString() const { return AlohalyticsBaseEvent::ToString() + " " + key; }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsBaseEvent::serialize(ar);
    ar(CEREAL_NVP(key));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyEvent, "k")

// Simple event with a string key/value pair.
struct AlohalyticsKeyValueEvent : public AlohalyticsKeyEvent {
  std::string value;

  virtual std::string ToString() const { return AlohalyticsKeyEvent::ToString() + " = " + value; }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsKeyEvent::serialize(ar);
    ar(CEREAL_NVP(value));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyValueEvent, "v")

// Simple event with a string key and map<string, string> value.
struct AlohalyticsKeyPairsEvent : public AlohalyticsKeyEvent {
  std::map<std::string, std::string> pairs;

  virtual std::string ToString() const {
    std::ostringstream stream;
    stream << AlohalyticsKeyEvent::ToString() << " [ ";
    for (const auto& pair : pairs) {
      stream << pair.first << '=' << pair.second << ' ';
    }
    stream << ']';
    return stream.str();
  }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsKeyEvent::serialize(ar);
    ar(CEREAL_NVP(pairs));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyPairsEvent, "p")

// Key + location.
struct AlohalyticsKeyLocationEvent : public AlohalyticsKeyEvent {
  alohalytics::Location location;

  virtual std::string ToString() const {
    return AlohalyticsKeyEvent::ToString() + ' ' + location.ToDebugString();
  }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsKeyEvent::serialize(ar);
    ar(CEREAL_NVP(location));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyLocationEvent, "kl")

// Key + value + location.
struct AlohalyticsKeyValueLocationEvent : public AlohalyticsKeyValueEvent {
  alohalytics::Location location;

  virtual std::string ToString() const {
    return AlohalyticsKeyValueEvent::ToString() + ' ' + location.ToDebugString();
  }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsKeyValueEvent::serialize(ar);
    ar(CEREAL_NVP(location));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyValueLocationEvent, "vl")

// Key + pairs of key/value + location.
struct AlohalyticsKeyPairsLocationEvent : public AlohalyticsKeyPairsEvent {
  alohalytics::Location location;

  virtual std::string ToString() const {
    return AlohalyticsKeyPairsEvent::ToString() + ' ' + location.ToDebugString();
  }

  template <class Archive>
  void serialize(Archive& ar) {
    AlohalyticsKeyPairsEvent::serialize(ar);
    ar(CEREAL_NVP(location));
  }
};
CEREAL_REGISTER_TYPE_WITH_NAME(AlohalyticsKeyPairsLocationEvent, "pl")

#endif  // EVENT_BASE_H
