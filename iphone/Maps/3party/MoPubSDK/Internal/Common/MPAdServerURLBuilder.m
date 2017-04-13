//
//  MPAdServerURLBuilder.m
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import "MPAdServerURLBuilder.h"

#import "MPConstants.h"
#import "MPGeolocationProvider.h"
#import "MPGlobal.h"
#import "MPKeywordProvider.h"
#import "MPIdentityProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPReachability.h"
#import "MPAPIEndpoints.h"

static NSString * const kMoPubInterfaceOrientationPortrait = @"p";
static NSString * const kMoPubInterfaceOrientationLandscape = @"l";
static NSInteger const kAdSequenceNone = -1;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerURLBuilder ()

+ (NSString *)queryParameterForKeywords:(NSString *)keywords;
+ (NSString *)queryParameterForOrientation;
+ (NSString *)queryParameterForScaleFactor;
+ (NSString *)queryParameterForTimeZone;
+ (NSString *)queryParameterForLocation:(CLLocation *)location;
+ (NSString *)queryParameterForMRAID;
+ (NSString *)queryParameterForDNT;
+ (NSString *)queryParameterForConnectionType;
+ (NSString *)queryParameterForApplicationVersion;
+ (NSString *)queryParameterForCarrierName;
+ (NSString *)queryParameterForISOCountryCode;
+ (NSString *)queryParameterForMobileNetworkCode;
+ (NSString *)queryParameterForMobileCountryCode;
+ (NSString *)queryParameterForDeviceName;
+ (NSString *)queryParameterForDesiredAdAssets:(NSArray *)assets;
+ (NSString *)queryParameterForAdSequence:(NSInteger)adSequence;
+ (NSString *)queryParameterForPhysicalScreenSize;
+ (NSString *)queryParameterForBundleIdentifier;
+ (NSString *)queryParameterForAppTransportSecurity;
+ (BOOL)advertisingTrackingEnabled;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdServerURLBuilder

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
                   testing:(BOOL)testing
{
    return [self URLWithAdUnitID:adUnitID
                        keywords:keywords
                        location:location
            versionParameterName:@"nv"
                         version:MP_SDK_VERSION
                         testing:testing
                   desiredAssets:nil];
}

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
      versionParameterName:(NSString *)versionParameterName
                   version:(NSString *)version
                   testing:(BOOL)testing
             desiredAssets:(NSArray *)assets
{


    return [self URLWithAdUnitID:adUnitID
                        keywords:keywords
                        location:location
            versionParameterName:versionParameterName
                         version:version
                         testing:testing
                   desiredAssets:assets
                      adSequence:kAdSequenceNone];
}

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
                  location:(CLLocation *)location
      versionParameterName:(NSString *)versionParameterName
                   version:(NSString *)version
                   testing:(BOOL)testing
             desiredAssets:(NSArray *)assets
                adSequence:(NSInteger)adSequence
{
    NSString *URLString = [NSString stringWithFormat:@"%@?v=%@&udid=%@&id=%@&%@=%@",
                           [MPAPIEndpoints baseURLStringWithPath:MOPUB_API_PATH_AD_REQUEST testing:testing],
                           MP_SERVER_VERSION,
                           [MPIdentityProvider identifier],
                           [adUnitID stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding],
                           versionParameterName, version];

    URLString = [URLString stringByAppendingString:[self queryParameterForKeywords:keywords]];
    URLString = [URLString stringByAppendingString:[self queryParameterForOrientation]];
    URLString = [URLString stringByAppendingString:[self queryParameterForScaleFactor]];
    URLString = [URLString stringByAppendingString:[self queryParameterForTimeZone]];
    URLString = [URLString stringByAppendingString:[self queryParameterForLocation:location]];
    URLString = [URLString stringByAppendingString:[self queryParameterForMRAID]];
    URLString = [URLString stringByAppendingString:[self queryParameterForDNT]];
    URLString = [URLString stringByAppendingString:[self queryParameterForConnectionType]];
    URLString = [URLString stringByAppendingString:[self queryParameterForApplicationVersion]];
    URLString = [URLString stringByAppendingString:[self queryParameterForCarrierName]];
    URLString = [URLString stringByAppendingString:[self queryParameterForISOCountryCode]];
    URLString = [URLString stringByAppendingString:[self queryParameterForMobileNetworkCode]];
    URLString = [URLString stringByAppendingString:[self queryParameterForMobileCountryCode]];
    URLString = [URLString stringByAppendingString:[self queryParameterForDeviceName]];
    URLString = [URLString stringByAppendingString:[self queryParameterForDesiredAdAssets:assets]];
    URLString = [URLString stringByAppendingString:[self queryParameterForAdSequence:adSequence]];
    URLString = [URLString stringByAppendingString:[self queryParameterForPhysicalScreenSize]];
    URLString = [URLString stringByAppendingString:[self queryParameterForBundleIdentifier]];
    URLString = [URLString stringByAppendingString:[self queryParameterForAppTransportSecurity]];

    return [NSURL URLWithString:URLString];
}


+ (NSString *)queryParameterForKeywords:(NSString *)keywords
{
    NSMutableArray *keywordsArray = [NSMutableArray array];
    NSString *trimmedKeywords = [keywords stringByTrimmingCharactersInSet:
                                 [NSCharacterSet whitespaceCharacterSet]];
    if ([trimmedKeywords length] > 0) {
        [keywordsArray addObject:trimmedKeywords];
    }

    // Append the Facebook attribution keyword (if available).
    Class fbKeywordProviderClass = NSClassFromString(@"MPFacebookKeywordProvider");
    if ([fbKeywordProviderClass conformsToProtocol:@protocol(MPKeywordProvider)])
    {
        NSString *fbAttributionKeyword = [(Class<MPKeywordProvider>) fbKeywordProviderClass keyword];
        if ([fbAttributionKeyword length] > 0) {
            [keywordsArray addObject:fbAttributionKeyword];
        }
    }

    if ([keywordsArray count] == 0) {
        return @"";
    } else {
        NSString *keywords = [[keywordsArray componentsJoinedByString:@","]
                              stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        return [NSString stringWithFormat:@"&q=%@", keywords];
    }
}

+ (NSString *)queryParameterForOrientation
{
    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
    NSString *orientString = UIInterfaceOrientationIsPortrait(orientation) ?
        kMoPubInterfaceOrientationPortrait : kMoPubInterfaceOrientationLandscape;
    return [NSString stringWithFormat:@"&o=%@", orientString];
}

+ (NSString *)queryParameterForScaleFactor
{
    return [NSString stringWithFormat:@"&sc=%.1f", MPDeviceScaleFactor()];
}

+ (NSString *)queryParameterForTimeZone
{
    static NSDateFormatter *formatter;
    @synchronized(self)
    {
        if (!formatter) formatter = [[NSDateFormatter alloc] init];
    }
    [formatter setDateFormat:@"Z"];
    NSDate *today = [NSDate date];
    return [NSString stringWithFormat:@"&z=%@", [formatter stringFromDate:today]];
}

+ (NSString *)queryParameterForLocation:(CLLocation *)location
{
    NSString *result = @"";

    CLLocation *bestLocation = location;
    CLLocation *locationFromProvider = [[[MPCoreInstanceProvider sharedProvider] sharedMPGeolocationProvider] lastKnownLocation];

    if (locationFromProvider) {
        bestLocation = locationFromProvider;
    }

    if (bestLocation && bestLocation.horizontalAccuracy >= 0) {
        result = [NSString stringWithFormat:@"&ll=%@,%@",
                  [NSNumber numberWithDouble:bestLocation.coordinate.latitude],
                  [NSNumber numberWithDouble:bestLocation.coordinate.longitude]];

        if (bestLocation.horizontalAccuracy) {
            result = [result stringByAppendingFormat:@"&lla=%@",
                      [NSNumber numberWithDouble:bestLocation.horizontalAccuracy]];
        }

        if (bestLocation == locationFromProvider) {
            result = [result stringByAppendingString:@"&llsdk=1"];
        }

        NSTimeInterval locationLastUpdatedMillis = [[NSDate date] timeIntervalSinceDate:bestLocation.timestamp] * 1000.0;

        result = [result stringByAppendingFormat:@"&llf=%.0f", locationLastUpdatedMillis];
    }

    return result;
}

+ (NSString *)queryParameterForMRAID
{
    if (NSClassFromString(@"MPMRAIDBannerCustomEvent") &&
        NSClassFromString(@"MPMRAIDInterstitialCustomEvent")) {
        return @"&mr=1";
    } else {
        return @"";
    }
}

+ (NSString *)queryParameterForDNT
{
    return [self advertisingTrackingEnabled] ? @"" : @"&dnt=1";
}

+ (NSString *)queryParameterForConnectionType
{
    return [[[MPCoreInstanceProvider sharedProvider] sharedMPReachability] hasWifi] ? @"&ct=2" : @"&ct=3";
}

+ (NSString *)queryParameterForApplicationVersion
{
    NSString *applicationVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    return [NSString stringWithFormat:@"&av=%@",
            [applicationVersion mp_URLEncodedString]];
}

+ (NSString *)queryParameterForCarrierName
{
    NSString *carrierName = [[[MPCoreInstanceProvider sharedProvider] sharedCarrierInfo] objectForKey:@"carrierName"];
    return carrierName ? [NSString stringWithFormat:@"&cn=%@",
                          [carrierName mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForISOCountryCode
{
    NSString *code = [[[MPCoreInstanceProvider sharedProvider] sharedCarrierInfo] objectForKey:@"isoCountryCode"];
    return code ? [NSString stringWithFormat:@"&iso=%@", [code mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForMobileNetworkCode
{
    NSString *code = [[[MPCoreInstanceProvider sharedProvider] sharedCarrierInfo] objectForKey:@"mobileNetworkCode"];
    return code ? [NSString stringWithFormat:@"&mnc=%@", [code mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForMobileCountryCode
{
    NSString *code = [[[MPCoreInstanceProvider sharedProvider] sharedCarrierInfo] objectForKey:@"mobileCountryCode"];
    return code ? [NSString stringWithFormat:@"&mcc=%@", [code mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForDeviceName
{
    NSString *deviceName = [[UIDevice currentDevice] mp_hardwareDeviceName];
    return deviceName ? [NSString stringWithFormat:@"&dn=%@", [deviceName mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForDesiredAdAssets:(NSArray *)assets
{
    NSString *concatenatedAssets = [assets componentsJoinedByString:@","];
    return [concatenatedAssets length] ? [NSString stringWithFormat:@"&assets=%@", concatenatedAssets] : @"";
}

+ (NSString *)queryParameterForAdSequence:(NSInteger)adSequence
{
    return (adSequence >= 0) ? [NSString stringWithFormat:@"&seq=%ld", (long)adSequence] : @"";
}

+ (NSString *)queryParameterForPhysicalScreenSize
{
    CGSize screenSize = MPScreenResolution();

    return [NSString stringWithFormat:@"&w=%.0f&h=%.0f", screenSize.width, screenSize.height];
}

+ (NSString *)queryParameterForBundleIdentifier
{
    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
    return bundleIdentifier ? [NSString stringWithFormat:@"&bundle=%@", [bundleIdentifier mp_URLEncodedString]] : @"";
}

+ (NSString *)queryParameterForAppTransportSecurity
{
    return [NSString stringWithFormat:@"&ats=%@", @([[MPCoreInstanceProvider sharedProvider] appTransportSecuritySettings])];
}

+ (BOOL)advertisingTrackingEnabled
{
    return [MPIdentityProvider advertisingTrackingEnabled];
}

@end
