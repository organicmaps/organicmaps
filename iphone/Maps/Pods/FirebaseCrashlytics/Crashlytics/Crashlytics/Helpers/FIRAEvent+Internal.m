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

#import "FIRAEvent+Internal.h"

#import "FIRCLSUtility.h"

NSString* FIRCLSFIRAEventDictionaryToJSON(NSDictionary* eventAsDictionary) {
  NSError* error = nil;

  if (eventAsDictionary == nil) {
    return nil;
  }

  if (![NSJSONSerialization isValidJSONObject:eventAsDictionary]) {
    FIRCLSSDKLog("Firebase Analytics event is not valid JSON");
    return nil;
  }

  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:eventAsDictionary
                                                     options:0
                                                       error:&error];

  if (error == nil) {
    NSString* json = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    return json;
  } else {
    FIRCLSSDKLog("Unable to convert Firebase Analytics event to json");
    return nil;
  }
}
