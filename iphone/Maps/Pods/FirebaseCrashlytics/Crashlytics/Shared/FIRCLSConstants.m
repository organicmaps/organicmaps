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

#import "FIRCLSConstants.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

NSString* const FIRCLSDeveloperToken = @"77f0789d8e230eccdb4b99b82dccd78d47f9b604";

NSString* const FIRCLSVersion = @STR(DISPLAY_VERSION);

// User Messages
NSString* const FIRCLSMissingConsumerKeyMsg = @"consumer key is nil or zero length";
NSString* const FIRCLSMissingConsumerSecretMsg = @"consumer secret is nil or zero length";

// Exceptions
NSString* const FIRCLSException = @"FIRCLSException";

// Endpoints
NSString* const FIRCLSSettingsEndpoint = @"https://firebase-settings.crashlytics.com";
NSString* const FIRCLSConfigureEndpoint = @"https://update.crashlytics.com";
NSString* const FIRCLSReportsEndpoint = @"https://reports.crashlytics.com";

// Network requests
NSString* const FIRCLSNetworkAccept = @"Accept";
NSString* const FIRCLSNetworkAcceptCharset = @"Accept-Charset";
NSString* const FIRCLSNetworkApplicationJson = @"application/json";
NSString* const FIRCLSNetworkAcceptLanguage = @"Accept-Language";
NSString* const FIRCLSNetworkContentLanguage = @"Content-Language";
NSString* const FIRCLSNetworkCrashlyticsAPIClientDisplayVersion =
    @"X-Crashlytics-API-Client-Display-Version";
NSString* const FIRCLSNetworkCrashlyticsAPIClientId = @"X-Crashlytics-API-Client-Id";
NSString* const FIRCLSNetworkCrashlyticsDeveloperToken = @"X-Crashlytics-Developer-Token";
NSString* const FIRCLSNetworkCrashlyticsGoogleAppId = @"X-Crashlytics-Google-App-Id";
NSString* const FIRCLSNetworkCrashlyticsOrgId = @"X-Crashlytics-Org-Id";
NSString* const FIRCLSNetworkUserAgent = @"User-Agent";
NSString* const FIRCLSNetworkUTF8 = @"utf-8";
