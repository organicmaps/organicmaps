//
//  MPIdentityProvider.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPIdentityProvider.h"
#import "MPGlobal.h"
#import "MPConsentManager.h"
#import <AdSupport/AdSupport.h>

#define MOPUB_IDENTIFIER_DEFAULTS_KEY @"com.mopub.identifier"
#define MOPUB_IDENTIFIER_LAST_SET_TIME_KEY @"com.mopub.identifiertime"
#define MOPUB_DAY_IN_SECONDS 24 * 60 * 60
#define MOPUB_ALL_ZERO_UUID @"00000000-0000-0000-0000-000000000000"
NSString *const mopubPrefix = @"mopub:";

static BOOL gFrequencyCappingIdUsageEnabled = YES;

@interface MPIdentityProvider ()
@property (class, nonatomic, readonly) NSCalendar * iso8601Calendar;

+ (NSString *)mopubIdentifier:(BOOL)obfuscate;

@end

@implementation MPIdentityProvider

+ (NSCalendar *)iso8601Calendar {
    static dispatch_once_t onceToken;
    static NSCalendar * _iso8601Calendar;
    dispatch_once(&onceToken, ^{
        _iso8601Calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierISO8601];
        _iso8601Calendar.timeZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
    });

    return _iso8601Calendar;
}

+ (NSString *)identifier
{
    return [self _identifier:NO];
}

+ (NSString *)obfuscatedIdentifier
{
    return [self _identifier:YES];
}

+ (NSString *)unobfuscatedMoPubIdentifier {
    NSString *value = [self mopubIdentifier:NO];
    if ([value hasPrefix:mopubPrefix]) {
        value = [value substringFromIndex:[mopubPrefix length]];
    }
    return value;
}

+ (NSString *)_identifier:(BOOL)obfuscate
{
    if (MPIdentityProvider.advertisingTrackingEnabled && [MPConsentManager sharedManager].canCollectPersonalInfo) {
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
    if (!MPIdentityProvider.advertisingTrackingEnabled) {
        return nil;
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

    // Compare the current timestamp to the timestamp of the last MoPub identifier generation.
    NSDate * now = [NSDate date];
    NSDate * lastSetDate = [[NSUserDefaults standardUserDefaults] objectForKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];

    // MoPub identifier has not been set before. Set the timestamp and let the identifer
    // be generated.
    if (lastSetDate == nil) {
        [[NSUserDefaults standardUserDefaults] setObject:now forKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
    // Current day does not match the same day when the identifier was generated.
    // Invalidate the current identifier so it can be regenerated.
    else if (![MPIdentityProvider.iso8601Calendar isDate:now inSameDayAsDate:lastSetDate]) {
        [[NSUserDefaults standardUserDefaults] setObject:now forKey:MOPUB_IDENTIFIER_LAST_SET_TIME_KEY];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
    }

    NSString * identifier = [[NSUserDefaults standardUserDefaults] objectForKey:MOPUB_IDENTIFIER_DEFAULTS_KEY];
    if (identifier == nil) {
        NSString *uuidStr = [[NSUUID UUID] UUIDString];

        identifier = [mopubPrefix stringByAppendingString:[uuidStr uppercaseString]];
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

@end
