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

#import "FIRCLSURLSessionAvailability.h"

#if FIRCLSURLSESSION_REQUIRED

#import <Foundation/Foundation.h>

@interface FIRCLSURLSessionConfiguration : NSObject <NSCopying> {
  NSDictionary *_additionalHeaders;
  NSURLCache *_URLCache;
  NSHTTPCookieAcceptPolicy _cookiePolicy;
}

+ (NSURLSessionConfiguration *)defaultSessionConfiguration;
+ (NSURLSessionConfiguration *)ephemeralSessionConfiguration;
+ (NSURLSessionConfiguration *)backgroundSessionConfiguration:(NSString *)identifier;
+ (NSURLSessionConfiguration *)backgroundSessionConfigurationWithIdentifier:(NSString *)identifier;

@property(nonatomic, copy) NSDictionary *HTTPAdditionalHeaders;
@property(nonatomic, retain) NSURLCache *URLCache;
@property(nonatomic, assign) NSHTTPCookieAcceptPolicy HTTPCookieAcceptPolicy;
@property(nonatomic, assign) BOOL sessionSendsLaunchEvents;
@property(nonatomic, assign) NSTimeInterval timeoutIntervalForRequest;
@property(nonatomic, assign) NSTimeInterval timeoutIntervalForResource;
@property(nonatomic, assign) BOOL allowsCellularAccess;

@end

#endif
