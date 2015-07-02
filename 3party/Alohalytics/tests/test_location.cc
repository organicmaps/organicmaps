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

#include "../src/location.h"

#include <sstream>
#include <memory>
#include <iostream>
#include <math.h>

using alohalytics::Location;
using std::cerr;
using std::endl;
using std::string;

TEST(Location, SmokeTest) {
  cerr.precision(20);
  cerr << std::fixed;

  const uint64_t timestamp = 1423169916587UL;
  const double lat = -84.123456789;
  const double lon = 177.123456789;
  const double horizontal_accuracy = 0.345678912345;
  const double alt = -11.123456789;
  const double vertical_accuracy = 0.987654321;
  const double speed = 27.123456789;
  const double bearing = 0.123456789;
  const Location::Source source = alohalytics::Location::Source::NETWORK;

  Location l0;
  const std::string serialized = l0.SetLatLon(timestamp, lat, lon, horizontal_accuracy)
                                     .SetAltitude(alt, vertical_accuracy)
                                     .SetSpeed(speed)
                                     .SetBearing(bearing)
                                     .SetSource(source)
                                     .Encode();
  EXPECT_EQ(serialized.size(), std::string::size_type(32));

  const alohalytics::Location l(serialized);
  EXPECT_TRUE(l.HasLatLon());
  EXPECT_EQ(l.timestamp_ms_, timestamp);
  EXPECT_NEAR(l.latitude_deg_, lat, 1e-7);
  EXPECT_NEAR(l.longitude_deg_, lon, 1e-7);
  EXPECT_NEAR(l.horizontal_accuracy_m_, horizontal_accuracy, 1e-2);
  EXPECT_TRUE(l.HasAltitude());
  EXPECT_NEAR(l.altitude_m_, alt, 1e-2);
  EXPECT_NEAR(l.vertical_accuracy_m_, vertical_accuracy, 1e-2);
  EXPECT_TRUE(l.HasBearing());
  EXPECT_NEAR(l.bearing_deg_, bearing, 1e-7);
  EXPECT_TRUE(l.HasSpeed());
  EXPECT_NEAR(l.speed_mps_, speed, 1e-2);
  EXPECT_TRUE(l.HasSource());
  EXPECT_EQ(l.source_, source);
}
