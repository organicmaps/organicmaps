//
//  MPWebBrowserUserAgentInfo.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MPWebBrowserUserAgentInfo : NSObject

/**
 The current user agent as determined by @c WKWebView.
 @returns The user agent.
*/
+ (NSString *)userAgent;

@end

NS_ASSUME_NONNULL_END
