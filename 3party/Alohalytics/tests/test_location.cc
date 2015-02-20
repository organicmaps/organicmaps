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
#include "../src/location.h"

#include <sstream>
#include <memory>
#include <iostream>
#include <math.h>

using alohalytics::Location;
using std::cerr;
using std::endl;
using std::string;

#define ASSERT_EQUAL(x, y)                                                                        \
  if ((x) != (y)) {                                                                               \
    cerr << "Test failed: " << #x << " != " << #y << " (" << (x) << " != " << (y) << ")" << endl; \
    return -1;                                                                                    \
  }
#define ASSERT_ALMOST_EQUAL(x, y, epsilon)                                                          \
  if (fabs((x) - (y)) > epsilon) {                                                                  \
    cerr << "Test failed: " << #x << " ~!= " << #y << " (" << (x) << " ~!= " << (y) << ")" << endl; \
    return -1;                                                                                      \
  }

int main(int, char **) {
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
  ASSERT_EQUAL(serialized.size(), 32);

  const alohalytics::Location l(serialized);
  ASSERT_EQUAL(l.HasLatLon(), true);
  ASSERT_EQUAL(l.timestamp_ms_, timestamp);
  ASSERT_ALMOST_EQUAL(l.latitude_deg_, lat, 1e-7);
  ASSERT_ALMOST_EQUAL(l.longitude_deg_, lon, 1e-7);
  ASSERT_ALMOST_EQUAL(l.horizontal_accuracy_m_, horizontal_accuracy, 1e-2);
  ASSERT_EQUAL(l.HasAltitude(), true);
  ASSERT_ALMOST_EQUAL(l.altitude_m_, alt, 1e-2);
  ASSERT_ALMOST_EQUAL(l.vertical_accuracy_m_, vertical_accuracy, 1e-2);
  ASSERT_EQUAL(l.HasBearing(), true);
  ASSERT_ALMOST_EQUAL(l.bearing_deg_, bearing, 1e-7);
  ASSERT_EQUAL(l.HasSpeed(), true);
  ASSERT_ALMOST_EQUAL(l.speed_mps_, speed, 1e-2);
  ASSERT_EQUAL(l.HasSource(), true);
  ASSERT_EQUAL(l.source_, source);

  std::cout << "All tests have passed." << endl;
  return 0;
}
