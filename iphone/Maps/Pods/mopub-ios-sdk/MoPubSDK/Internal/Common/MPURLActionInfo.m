//
//  MPURLActionInfo.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPURLActionInfo.h"

@interface MPURLActionInfo ()

@property (nonatomic, readwrite) MPURLActionType actionType;
@property (nonatomic, readwrite, copy) NSURL *originalURL;
@property (nonatomic, readwrite, strong) NSDictionary *iTunesStoreParameters;
@property (nonatomic, readwrite, copy) NSURL *iTunesStoreFallbackURL;
@property (nonatomic, readwrite, copy) NSURL *safariDestinationURL;
@property (nonatomic, readwrite, copy) NSString *HTTPResponseString;
@property (nonatomic, readwrite, copy) NSURL *webViewBaseURL;
@property (nonatomic, readwrite, copy) NSURL *deeplinkURL;
@property (nonatomic, readwrite, strong) MPEnhancedDeeplinkRequest *enhancedDeeplinkRequest;
@property (nonatomic, readwrite, copy) NSURL *shareURL;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPURLActionInfo

+ (instancetype)infoWithURL:(NSURL *)URL iTunesStoreParameters:(NSDictionary *)parameters iTunesStoreFallbackURL:(NSURL *)fallbackURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeStoreKit;
    info.originalURL = URL;
    info.iTunesStoreParameters = parameters;
    info.iTunesStoreFallbackURL = fallbackURL;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL safariDestinationURL:(NSURL *)safariDestinationURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeOpenInSafari;
    info.originalURL = URL;
    info.safariDestinationURL = safariDestinationURL;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL HTTPResponseString:(NSString *)responseString webViewBaseURL:(NSURL *)baseURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeOpenInWebView;
    info.originalURL = URL;
    info.HTTPResponseString = responseString;
    info.webViewBaseURL = baseURL;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL webViewBaseURL:(NSURL *)baseURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeOpenURLInWebView;
    info.originalURL = URL;
    info.webViewBaseURL = baseURL;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL deeplinkURL:(NSURL *)deeplinkURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeGenericDeeplink;
    info.originalURL = URL;
    info.deeplinkURL = deeplinkURL;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL enhancedDeeplinkRequest:(MPEnhancedDeeplinkRequest *)request
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeEnhancedDeeplink;
    info.originalURL = URL;
    info.enhancedDeeplinkRequest = request;
    return info;
}

+ (instancetype)infoWithURL:(NSURL *)URL shareURL:(NSURL *)shareURL
{
    MPURLActionInfo *info = [[[self class] alloc] init];
    info.actionType = MPURLActionTypeShare;
    info.originalURL = URL;
    info.shareURL = shareURL;
    return info;
}

@end
