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

#include "gtest/gtest.h"

#include "../src/file_manager.h"
#include "../src/gzip_wrapper.h"
#include "../server/statistics_receiver.h"

using alohalytics::FileManager;
using alohalytics::Gzip;
using alohalytics::NoOpDeleter;
using alohalytics::ScopedRemoveFile;
using alohalytics::StatisticsReceiver;

using namespace std;

static constexpr const char * kTestDirectory = ".";
static const string kQueueFileToCleanUp =
    string(kTestDirectory) + FileManager::kDirectorySeparator + alohalytics::kCurrentFileName;

static constexpr const char * kFirstEventId = "First Unique ID";
static constexpr const char * kSecondEventId = "Second Unique ID";
static constexpr const char * kFirstIP = "192.168.0.1";
static constexpr const char * kSecondIP = "192.168.1.100";
static constexpr const char * kFirstUA = "Test User Agent";
static constexpr const char * kSecondUA = "Another Test User Agent";
static constexpr const char * kFirstURI = "/test/uri";
static constexpr const char * kSecondURI = "/second/test/uri";

string CreateCerealIdEvent(const char * id) {
  AlohalyticsIdEvent event;
  event.id = id;
  std::ostringstream sstream;
  cereal::BinaryOutputArchive(sstream) << unique_ptr<AlohalyticsBaseEvent const, NoOpDeleter>(&event);
  return sstream.str();
}

TEST(StatisticsReceiver, SmokeTest) {
  ScopedRemoveFile remover(kQueueFileToCleanUp);
  {
    StatisticsReceiver receiver(kTestDirectory);
    receiver.ProcessReceivedHTTPBody(Gzip(CreateCerealIdEvent(kFirstEventId)), AlohalyticsBaseEvent::CurrentTimestamp(),
                                     kFirstIP, kFirstUA, kFirstURI);
    receiver.ProcessReceivedHTTPBody(Gzip(CreateCerealIdEvent(kSecondEventId)),
                                     AlohalyticsBaseEvent::CurrentTimestamp(), kSecondIP, kSecondUA, kSecondURI);
  }
  // Check that all events were correctly stored in the queue.
  {
    const string cereal_binary_events = FileManager::ReadFileAsString(kQueueFileToCleanUp);
    istringstream in_stream(cereal_binary_events);
    cereal::BinaryInputArchive in_ar(in_stream);
    unique_ptr<AlohalyticsBaseEvent> ptr;
    in_ar(ptr);
    const AlohalyticsIdServerEvent * id_event = dynamic_cast<const AlohalyticsIdServerEvent *>(ptr.get());
    EXPECT_NE(nullptr, id_event);
    EXPECT_EQ(id_event->id, kFirstEventId);
    EXPECT_EQ(id_event->ip, kFirstIP);
    EXPECT_EQ(id_event->user_agent, kFirstUA);
    EXPECT_EQ(id_event->uri, kFirstURI);
    in_ar(ptr);
    id_event = dynamic_cast<const AlohalyticsIdServerEvent *>(ptr.get());
    EXPECT_NE(nullptr, id_event);
    EXPECT_EQ(id_event->id, kSecondEventId);
    EXPECT_EQ(id_event->ip, kSecondIP);
    EXPECT_EQ(id_event->user_agent, kSecondUA);
    EXPECT_EQ(id_event->uri, kSecondURI);
    EXPECT_EQ(static_cast<size_t>(in_stream.tellg()), cereal_binary_events.size());
  }
}
