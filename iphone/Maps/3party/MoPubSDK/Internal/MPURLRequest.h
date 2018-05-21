//
//  MPURLRequest.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
@interface MPURLRequest : NSMutableURLRequest

/**
 Initializes an URL request with a given URL.
 @param URL The URL for the request.
 @returns Returns a URL request for a specified URL with @c NSURLRequestReloadIgnoringCacheData
 cache policy and @c kRequestTimeoutInterval timeout value.
 */
- (instancetype)initWithURL:(NSURL *)URL;

/**
 Initializes an URL request with a given URL.
 @param URL The URL for the request.
 @returns Returns a URL request for a specified URL with @c NSURLRequestReloadIgnoringCacheData
 cache policy and @c kRequestTimeoutInterval timeout value.
 */
+ (MPURLRequest *)requestWithURL:(NSURL *)URL;

@end
NS_ASSUME_NONNULL_END
