//
//  MPIdentityProvider.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPIdentityProvider.h"
#import "MPGlobal.h"
#import <AdSupport/AdSupport.h>

#define MOPUB_IDENTIFIER_DEFAULTS_KEY @"com.mopub.identifier"
#define MOPUB_IDENTIFIER_LAST_SET_TIME_KEY @"com.mopub.identifiertime"
#define MOPUB_DAY_IN_SECONDS 24 * 60 * 60
#define MOPUB_ALL_ZERO_UUID @"00000000-0000-0000-0000-000000000000"

static BOOL gFrequencyCappingIdUsageEnabled = YES;

@interface MPIdentityProvider ()

+ (NSString *)identifierFromASIdentifierManager:(BOOL)obfuscate;
+ (NSString *)mopubIdentifier:(BOOL)obfuscate;

@end

@implementation MPIdentityProvider

+ (NSString *)identifier
{
    return [self _identifier:NO];
}

+ (NSString *)obfuscatedIdentifier
{
    return [self _identifier:YES];
}

+ (NSString *)_identifier:(BOOL)obfuscate
{
    if (![self isAdvertisingIdAllZero]) {
        return [self identifierFromASIdentifierManager:obfuscate];
    } else {
        return [self mopubIdentifier:obfuscate];
    }
}

+ (BOOL)advertisingTrackingEnabled
{
    return [[ASIdentifierManager sharedManager] isAdvertisingTrackingEnabled];
}

+ (NSString *)identifierFromASIdentifierManager:(BOOL)obfuscate
{
    if (obfuscate) {
        return @"ifa:XXXX";
    }

    NSString *identifier = [[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString];

    return [NSString stringWithFormat:@"ifa:%@", [identifier uppercaseString]];
}

+ (NSString *)mopubIdentifier:(BOOL)obfuscate
{
    if (![self frequencyCappingIdUsageEnabled]) {
        return [NSString stringWithFormat:@"ifa:%@", MOPUB_ALL_ZERO_UUID];
    }

    if (obfuscate) {
        return @"mopub:XXXX";
    }

    // reset identifier every 24 hours
    NSDate *lastSetDate = [[NSUserDefaults standardUserDefaults] objectForKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];
    if (!lastSetDate) {
        [[NSUserDefaults standardUserDefaults] setObject:[NSDate date] forKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];
        [[NSUserDefaults standardUserDefaults] synchronize];
    } else {
        NSTimeInterval diff = [[NSDate date] timeIntervalSinceDate:lastSetDate];
        if (diff > MOPUB_DAY_IN_SECONDS) {
            [[NSUserDefaults standardUserDefaults] setObject:[NSDate date] forKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
        }
    }

    NSString *identifier = [[NSUserDefaults standardUserDefaults] objectForKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
    if (!identifier) {
        CFUUIDRef uuidObject = CFUUIDCreate(kCFAllocatorDefault);
        NSString *uuidStr = (NSString *)CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuidObject));
        CFRelease(uuidObject);

        identifier = [NSString stringWithFormat:@"mopub:%@", [uuidStr uppercaseString]];
        [[NSUserDefaults standardUserDefaults] setObject:identifier forKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }

    return identifier;
}

+ (void)setFrequencyCappingIdUsageEnabled:(BOOL)frequencyCappingIdUsageEnabled
{
    gFrequencyCappingIdUsageEnabled = frequencyCappingIdUsageEnabled;
}

+ (BOOL)frequencyCappingIdUsageEnabled
{
    return gFrequencyCappingIdUsageEnabled;
}



// Beginning in iOS 10, when a user enables "Limit Ad Tracking", the OS will send advertising identifier with value of
// 00000000-0000-0000-0000-000000000000

+ (BOOL)isAdvertisingIdAllZero {
    NSString *identifier = [[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString];
    if (!identifier) {
        // when identifier is nil, ifa:(null) is sent.
        return false;
    }  else {
        return [identifier isEqualToString:MOPUB_ALL_ZERO_UUID];
    }
}

@end
