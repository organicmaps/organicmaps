//
//  MPAPIEndpoints.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAPIEndpoints.h"
#import "MPConstants.h"
#import "MPDeviceInformation.h"

// URL scheme constants
static NSString * const kUrlSchemeHttp = @"http";
static NSString * const kUrlSchemeHttps = @"https";

// Base URL constant
static NSString * const kMoPubBaseHostname = @"ads.mopub.com";

@implementation MPAPIEndpoints

#pragma mark - baseHostname property

static NSString * _baseHostname = nil;
+ (void)setBaseHostname:(NSString *)baseHostname {
    _baseHostname = baseHostname;
}

+ (NSString *)baseHostname {
    if (_baseHostname == nil || [_baseHostname isEqualToString:@""]) {
        return kMoPubBaseHostname;
    }

    return _baseHostname;
}

#pragma mark - setUsesHTTPS

static BOOL sUsesHTTPS = YES;
+ (void)setUsesHTTPS:(BOOL)usesHTTPS
{
    sUsesHTTPS = usesHTTPS;
}

#pragma mark - baseURL

+ (NSString *)baseURL
{
    if (MPDeviceInformation.appTransportSecuritySettings == MPATSSettingEnabled) {
        return [@"https://" stringByAppendingString:self.baseHostname];
    }

    return [@"http://" stringByAppendingString:self.baseHostname];
}

+ (NSURLComponents *)baseURLComponentsWithPath:(NSString *)path
{
    NSURLComponents * components = [[NSURLComponents alloc] init];
    components.scheme = (sUsesHTTPS ? kUrlSchemeHttps : kUrlSchemeHttp);
    components.host = self.baseHostname;
    components.path = path;

    return components;
}

@end
