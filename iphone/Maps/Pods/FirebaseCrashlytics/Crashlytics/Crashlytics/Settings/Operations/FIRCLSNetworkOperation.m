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

#import "FIRCLSNetworkOperation.h"

#import "FIRCLSApplication.h"
#import "FIRCLSConstants.h"
#import "FIRCLSDataCollectionToken.h"
#import "FIRCLSDefines.h"
#import "FIRCLSLogger.h"

@interface FIRCLSNetworkOperation ()

@property(nonatomic, strong, readonly) NSString *googleAppID;

@end

@implementation FIRCLSNetworkOperation

- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                              token:(FIRCLSDataCollectionToken *)token {
  NSParameterAssert(googleAppID);
  if (!googleAppID) {
    return nil;
  }

  self = [super init];
  if (self) {
    _googleAppID = googleAppID;
    _token = token;
  }
  return self;
}

- (void)startWithToken:(FIRCLSDataCollectionToken *)token {
  // Settings and Onboarding are considered data collection, so we must only
  // call this with a valid token
  if (![token isValid]) {
    FIRCLSErrorLog(@"Skipping network operation with invalid data collection token");
    return;
  }

  [super start];
}

- (NSMutableURLRequest *)mutableRequestWithDefaultHTTPHeaderFieldsAndTimeoutForURL:(NSURL *)url {
  return [self mutableRequestWithDefaultHTTPHeadersForURL:url timeout:10.0];
}

- (NSMutableURLRequest *)mutableRequestWithDefaultHTTPHeadersForURL:(NSURL *)url
                                                            timeout:(NSTimeInterval)timeout {
  NSMutableURLRequest *request =
      [NSMutableURLRequest requestWithURL:url
                              cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                          timeoutInterval:timeout];

  NSString *localeId = self.localeIdentifier;

  [request setValue:self.userAgentString forHTTPHeaderField:FIRCLSNetworkUserAgent];
  [request setValue:FIRCLSNetworkUTF8 forHTTPHeaderField:FIRCLSNetworkAcceptCharset];
  [request setValue:localeId forHTTPHeaderField:FIRCLSNetworkAcceptLanguage];
  [request setValue:localeId forHTTPHeaderField:FIRCLSNetworkContentLanguage];
  [request setValue:FIRCLSDeveloperToken forHTTPHeaderField:FIRCLSNetworkCrashlyticsDeveloperToken];
  [request setValue:FIRCLSApplicationGetSDKBundleID()
      forHTTPHeaderField:FIRCLSNetworkCrashlyticsAPIClientId];
  [request setValue:FIRCLSVersion
      forHTTPHeaderField:FIRCLSNetworkCrashlyticsAPIClientDisplayVersion];
  [request setValue:self.googleAppID forHTTPHeaderField:FIRCLSNetworkCrashlyticsGoogleAppId];

  return request;
}

- (NSString *)userAgentString {
  return [NSString stringWithFormat:@"%@/%@", FIRCLSApplicationGetSDKBundleID(), FIRCLSVersion];
}

- (NSString *)localeIdentifier {
  return NSLocale.currentLocale.localeIdentifier;
}

@end
