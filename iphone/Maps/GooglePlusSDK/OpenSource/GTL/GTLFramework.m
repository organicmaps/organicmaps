/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GTLFramework.h"

void GTLFrameworkVersion(NSUInteger* major, NSUInteger* minor, NSUInteger* release) {
  // version 2.0.0
  if (major)   *major = 2;
  if (minor)   *minor = 0;
  if (release) *release = 0;
}

NSString *GTLFrameworkVersionString(void) {
  NSUInteger major, minor, release;
  NSString *libVersionString;

  GTLFrameworkVersion(&major, &minor, &release);

  // most library releases will have a release value of zero
  if (release != 0) {
    libVersionString = [NSString stringWithFormat:@"%d.%d.%d",
                        (int)major, (int)minor, (int)release];
  } else {
    libVersionString = [NSString stringWithFormat:@"%d.%d",
                        (int)major, (int)minor];
  }
  return libVersionString;
}
