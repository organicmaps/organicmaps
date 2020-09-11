//
//  MPWebBrowserUserAgentInfo.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <WebKit/WebKit.h>
#import "MPLogging.h"
#import "MPWebBrowserUserAgentInfo.h"

/**
 Global variable for holding the user agent string.
 */
NSString *gUserAgent = nil;

/**
 Global variable for keeping `WKWebView` alive until the async call for user agent finishes.
 Note: JavaScript evaluation will fail if the `WKWebView` is deallocated before completion.
 */
WKWebView *gWkWebView = nil;

/**
 The `UserDefaults` key for accessing the cached user agent value.
 */
NSString * const kUserDefaultsUserAgentKey = @"com.mopub.mopub-ios-sdk.user-agent";

@implementation MPWebBrowserUserAgentInfo

+ (void)load {
    // No need for "dispatch once" since `load` is called only once during app launch.
    [self obtainUserAgentFromWebView];
}

+ (void)obtainUserAgentFromWebView {
    NSString *cachedUserAgent = [NSUserDefaults.standardUserDefaults stringForKey:kUserDefaultsUserAgentKey];
    if (cachedUserAgent.length > 0) {
        // Use the cached value before the async JavaScript evaluation is successful.
        gUserAgent = cachedUserAgent;
    } else {
        /*
         Use the composed value before the async JavaScript evaluation is successful. This composed
         user agent value should be very close to the actual value like this one:
           "Mozilla/5.0 (iPhone; CPU iPhone OS 12_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148"
         The history of user agent is very long, complicated, and confusing. Please search online to
         learn about why the user agent value looks like this.
        */

        NSString *systemVersion = [[UIDevice currentDevice].systemVersion stringByReplacingOccurrencesOfString:@"." withString:@"_"];
        NSString *deviceType = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? @"iPad" : @"iPhone";
        gUserAgent = [NSString stringWithFormat:@"Mozilla/5.0 (%@; CPU %@ OS %@ like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148",
                      deviceType, deviceType, systemVersion];
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        gWkWebView = [WKWebView new]; // `WKWebView` must be created in main thread
        [gWkWebView evaluateJavaScript:@"navigator.userAgent" completionHandler:^(id _Nullable result, NSError * _Nullable error) {
            if (error != nil) {
                MPLogInfo(@"%@ error: %@", NSStringFromSelector(_cmd), error);
            } else if ([result isKindOfClass:NSString.class]) {
                gUserAgent = result;
                [NSUserDefaults.standardUserDefaults setValue:result forKeyPath:kUserDefaultsUserAgentKey];
            }
            gWkWebView = nil;
        }];
    });
}

+ (NSString *)userAgent {
    return gUserAgent;
}

@end
