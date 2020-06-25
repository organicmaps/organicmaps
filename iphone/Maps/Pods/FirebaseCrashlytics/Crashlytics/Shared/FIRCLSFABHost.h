// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>

/**
 * Returns the OS version of the host device
 */
NSOperatingSystemVersion FIRCLSHostGetOSVersion(void);

/**
 * Returns model info for the device on which app is running
 */
NSString *FIRCLSHostModelInfo(void);

/**
 * Returns a string representing the OS build
 */
NSString *FIRCLSHostOSBuildVersion(void);

/**
 * Returns a concatenated string of the OS version(majorVersion.minorVersion.patchVersion)
 */
NSString *FIRCLSHostOSDisplayVersion(void);
