//
//  MPURLActionInfo.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

#import "MPEnhancedDeeplinkRequest.h"

#ifndef NS_ENUM
#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
#endif

typedef NS_ENUM(NSUInteger, MPURLActionType) {
    MPURLActionTypeStoreKit,
    MPURLActionTypeGenericDeeplink,
    MPURLActionTypeEnhancedDeeplink,
    MPURLActionTypeOpenInSafari,
    MPURLActionTypeOpenURLInWebView,
    MPURLActionTypeOpenInWebView,
    MPURLActionTypeShare
};

@interface MPURLActionInfo : NSObject

@property (nonatomic, readonly) MPURLActionType actionType;
@property (nonatomic, readonly, copy) NSURL *originalURL;
@property (nonatomic, readonly, strong) NSDictionary *iTunesStoreParameters;
@property (nonatomic, readonly, copy) NSURL *iTunesStoreFallbackURL;
@property (nonatomic, readonly, copy) NSURL *safariDestinationURL;
@property (nonatomic, readonly, copy) NSString *HTTPResponseString;
@property (nonatomic, readonly, copy) NSURL *webViewBaseURL;
@property (nonatomic, readonly, copy) NSURL *deeplinkURL;
@property (nonatomic, readonly, strong) MPEnhancedDeeplinkRequest *enhancedDeeplinkRequest;
@property (nonatomic, readonly, copy) NSURL *shareURL;

+ (instancetype)infoWithURL:(NSURL *)URL iTunesStoreParameters:(NSDictionary *)parameters iTunesStoreFallbackURL:(NSURL *)fallbackURL;
+ (instancetype)infoWithURL:(NSURL *)URL safariDestinationURL:(NSURL *)safariDestinationURL;
+ (instancetype)infoWithURL:(NSURL *)URL HTTPResponseString:(NSString *)responseString webViewBaseURL:(NSURL *)baseURL;
+ (instancetype)infoWithURL:(NSURL *)URL webViewBaseURL:(NSURL *)baseURL;
+ (instancetype)infoWithURL:(NSURL *)URL deeplinkURL:(NSURL *)deeplinkURL;
+ (instancetype)infoWithURL:(NSURL *)URL enhancedDeeplinkRequest:(MPEnhancedDeeplinkRequest *)request;
+ (instancetype)infoWithURL:(NSURL *)URL shareURL:(NSURL *)shareURL;

@end
