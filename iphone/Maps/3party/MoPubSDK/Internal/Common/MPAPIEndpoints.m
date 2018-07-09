//
//  MPAPIEndpoints.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPAPIEndpoints.h"
#import "MPConstants.h"
#import "MPCoreInstanceProvider.h"

@implementation MPAPIEndpoints

static BOOL sUsesHTTPS = YES;

+ (void)setUsesHTTPS:(BOOL)usesHTTPS
{
    sUsesHTTPS = usesHTTPS;
}

+ (NSString *)baseURL
{
    if ([[MPCoreInstanceProvider sharedProvider] appTransportSecuritySettings] == MPATSSettingEnabled) {
        return [@"https://" stringByAppendingString:MOPUB_BASE_HOSTNAME];
    }

    return [@"http://" stringByAppendingString:MOPUB_BASE_HOSTNAME];
}

+ (NSString *)baseURLScheme
{
    return sUsesHTTPS ? @"https://" : @"http://";
}

+ (NSString *)baseURLStringWithPath:(NSString *)path
{
    return [NSString stringWithFormat:@"%@%@%@",
            [[self class] baseURLScheme],
            MOPUB_BASE_HOSTNAME,
            path];
}

@end
