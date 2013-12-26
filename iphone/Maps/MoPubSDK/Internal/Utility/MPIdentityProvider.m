//
//  MPIdentityProvider.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPIdentityProvider.h"
#import "MPGlobal.h"

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
#import <AdSupport/AdSupport.h>
#endif

#define MOPUB_IDENTIFIER_DEFAULTS_KEY @"com.mopub.identifier"

@interface MPIdentityProvider ()

+ (BOOL)deviceHasASIdentifierManager;

+ (NSString *)identifierFromASIdentifierManager;
+ (NSString *)mopubIdentifier;

@end

@implementation MPIdentityProvider

+ (BOOL)deviceHasASIdentifierManager
{
    return !!NSClassFromString(@"ASIdentifierManager");
}

+ (NSString *)identifier
{
    if ([self deviceHasASIdentifierManager]) {
        return [self identifierFromASIdentifierManager];
    } else {
        return [self mopubIdentifier];
    }
}

+ (BOOL)advertisingTrackingEnabled
{
    BOOL enabled = YES;

    if ([self deviceHasASIdentifierManager]) {
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
        enabled = [[ASIdentifierManager sharedManager] isAdvertisingTrackingEnabled];
#endif
    }

    return enabled;
}

+ (NSString *)identifierFromASIdentifierManager
{
    NSString *identifier = nil;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
    identifier = [[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString];
#endif
    return [NSString stringWithFormat:@"ifa:%@", [identifier uppercaseString]];
}

+ (NSString *)mopubIdentifier
{
    NSString *identifier = [[NSUserDefaults standardUserDefaults] objectForKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
    if (!identifier) {
        CFUUIDRef uuidObject = CFUUIDCreate(kCFAllocatorDefault);
        NSString *uuidStr = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuidObject);
        CFRelease(uuidObject);
        [uuidStr autorelease];

        identifier = [NSString stringWithFormat:@"mopub:%@", [uuidStr uppercaseString]];
        [[NSUserDefaults standardUserDefaults] setObject:identifier forKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
    return identifier;
}

@end
