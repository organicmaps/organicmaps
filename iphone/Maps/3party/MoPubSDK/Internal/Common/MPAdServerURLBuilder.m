//
//  MPAdServerURLBuilder.m
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import "MPAdServerURLBuilder.h"

#import "MPAdvancedBiddingManager.h"
#import "MPConsentAdServerKeys.h"
#import "MPConstants.h"
#import "MPGeolocationProvider.h"
#import "MPGlobal.h"
#import "MPIdentityProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPReachability.h"
#import "MPAPIEndpoints.h"
#import "MPViewabilityTracker.h"
#import "NSString+MPAdditions.h"
#import "NSString+MPConsentStatus.h"
#import "MPConsentManager.h"

static NSString * const kMoPubInterfaceOrientationPortrait = @"p";
static NSString * const kMoPubInterfaceOrientationLandscape = @"l";
static NSInteger const kAdSequenceNone = -1;

// Ad Server URL Keys
static NSString * const kServerAPIVersionKey = @"v";
static NSString * const kApplicationVersionKey = @"av";

// Open Endpoint-specific Keys
static NSString * const kOpenEndpointIDKey = @"id";
static NSString * const kOpenEndpointSessionTrackingKey = @"st";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerURLBuilder ()

+ (NSString *)queryParameterForKeywords:(NSString *)keywords;
+ (NSString *)queryParameterForUserDataKeywords:(NSString *)userDataKeywords;
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
+ (NSString *)queryParameterForConsent;
+ (BOOL)advertisingTrackingEnabled;

// Helper methods
+ (NSString *)URLEncodedApplicationVersion;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdServerURLBuilder

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location
{
    return [self URLWithAdUnitID:adUnitID
                        keywords:keywords
                userDataKeywords:userDataKeywords
                        location:location
                   desiredAssets:nil
                     viewability:YES];
}

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location
             desiredAssets:(NSArray *)assets
               viewability:(BOOL)viewability
{


    return [self URLWithAdUnitID:adUnitID
                        keywords:keywords
                userDataKeywords:userDataKeywords
                        location:location
                   desiredAssets:assets
                      adSequence:kAdSequenceNone
                     viewability:viewability];
}

+ (NSURL *)URLWithAdUnitID:(NSString *)adUnitID
                  keywords:(NSString *)keywords
          userDataKeywords:(NSString *)userDataKeywords
                  location:(CLLocation *)location
             desiredAssets:(NSArray *)assets
                adSequence:(NSInteger)adSequence
               viewability:(BOOL)viewability
{
    NSString *URLString = [NSString stringWithFormat:@"%@?%@=%@&udid=%@&id=%@&nv=%@",
                           [MPAPIEndpoints baseURLStringWithPath:MOPUB_API_PATH_AD_REQUEST],
                           kServerAPIVersionKey,
                           MP_SERVER_VERSION,
                           [MPIdentityProvider identifier],
                           [adUnitID stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding],
                           MP_SDK_VERSION];
    URLString = [URLString stringByAppendingString:[self queryParameterForOrientation]];
    URLString = [URLString stringByAppendingString:[self queryParameterForScaleFactor]];
    URLString = [URLString stringByAppendingString:[self queryParameterForTimeZone]];
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

    if ([MPConsentManager sharedManager].canCollectPersonalInfo) {
        URLString = [URLString stringByAppendingString:[self queryParameterForLocation:location]];
    }

    // If a user is in GDPR region and MoPub doesn't obtain consent from the user, personal keywords won't be sent
    // to the server. It's handled in `queryParameterForKeywords` method.
    URLString = [URLString stringByAppendingString:[self queryParameterForKeywords:keywords]];
    URLString = [URLString stringByAppendingString:[self queryParameterForUserDataKeywords:userDataKeywords]];

    if (viewability) {
        URLString = [URLString stringByAppendingString:[self queryParameterForViewability]];
    }

    NSString * advancedBiddingQueryParameter = [self queryParameterForAdvancedBidding];
    if (advancedBiddingQueryParameter) {
        URLString = [URLString stringByAppendingString:advancedBiddingQueryParameter];
    }

    URLString = [URLString stringByAppendingString:[self queryParameterForConsent]];

    // In the event that the `adUnitIdUsedForConsent` from `MPConsentManager` is still `nil`,
    // we should populate it with this `adUnitId`. This is to cover the edge case where the
    // publisher does not explcitily initialize the SDK via `initializeSdkWithConfiguration:completion:`.
    if (adUnitID != nil && MPConsentManager.sharedManager.adUnitIdUsedForConsent == nil) {
        MPConsentManager.sharedManager.adUnitIdUsedForConsent = adUnitID;
    }

    return [NSURL URLWithString:URLString];
}


+ (NSString *)queryParameterForUserDataKeywords:(NSString *)userDataKeywords
{
    if (![MPConsentManager sharedManager].canCollectPersonalInfo) {
        return @"";
    }
    return [MPAdServerURLBuilder queryParameterForKeywords:userDataKeywords key:@"user_data_q"];
}

+ (NSString *)queryParameterForKeywords:(NSString *)keywords
{
    return [MPAdServerURLBuilder queryParameterForKeywords:keywords key:@"q"];
}

+ (NSString *)queryParameterForKeywords:(NSString *)keywords key:(NSString *)key
{
    NSString *trimmedKeywords = [keywords stringByTrimmingCharactersInSet:
                                 [NSCharacterSet whitespaceCharacterSet]];
    if ([trimmedKeywords length] > 0) {
        NSString *keywords = [trimmedKeywords stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        return [NSString stringWithFormat:@"&%@=%@", key, keywords];
    }
    return @"";
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
    return [NSString stringWithFormat:@"&%@=%@", kApplicationVersionKey, [self URLEncodedApplicationVersion]];
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

+ (NSString *)queryParameterForViewability {
    return [NSString stringWithFormat:@"&vv=%d", (int)[MPViewabilityTracker enabledViewabilityVendors]];
}

+ (NSString *)queryParameterForAdvancedBidding {
    // Opted out of advanced bidding, no query parameter should be sent.
    if (![MPAdvancedBiddingManager sharedManager].advancedBiddingEnabled) {
        return nil;
    }

    // No JSON at this point means that no advanced bidders were initialized.
    NSString * tokens = MPAdvancedBiddingManager.sharedManager.bidderTokensJson;
    if (tokens == nil) {
        return nil;
    }

    // URL encode the JSON string
    NSString * urlEncodedTokens = [tokens mp_URLEncodedString];
    if (urlEncodedTokens == nil) {
        return nil;
    }

    return [NSString stringWithFormat:@"&abt=%@", urlEncodedTokens];
}

+ (NSString *)queryParameterForConsent {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    // Consent state
    NSString * consent = [NSString stringFromConsentStatus:manager.currentStatus];
    NSMutableString * consentQuery = [NSMutableString stringWithFormat:@"&%@=%@", kCurrentConsentStatusKey, consent];

    // GDPR applicable state
    if (manager.isGDPRApplicable != MPBoolUnknown) {
        NSString * gdprBoolValue = manager.isGDPRApplicable > 0 ? @"1" : @"0";
        [consentQuery appendFormat:@"&%@=%@", kGDPRAppliesKey, gdprBoolValue];
    }

    // User consented versions
    if (manager.consentedPrivacyPolicyVersion != nil) {
        [consentQuery appendFormat:@"&%@=%@", kConsentedPrivacyPolicyVersionKey, manager.consentedPrivacyPolicyVersion];
    }

    if (manager.consentedVendorListVersion != nil) {
        [consentQuery appendFormat:@"&%@=%@", kConsentedVendorListVersionKey, manager.consentedVendorListVersion];
    }

    return consentQuery;
}

+ (BOOL)advertisingTrackingEnabled
{
    return [MPIdentityProvider advertisingTrackingEnabled];
}

+ (NSString *)URLEncodedApplicationVersion {
    static NSString * gApplicationVersion;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        gApplicationVersion = [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"] mp_URLEncodedString];
    });

    return gApplicationVersion;
}

@end

@interface MPAdServerURLBuilder (Helper)

+ (NSURL *)URLWithEndpointPath:(NSString *)endpointPath queryParameters:(NSDictionary *)parameters;

+ (NSString *)queryItemForKey:(NSString *)key value:(NSString *)value;

@end

@implementation MPAdServerURLBuilder (Helper)

+ (NSURL *)URLWithEndpointPath:(NSString *)endpointPath queryParameters:(NSDictionary *)parameters {
    // Build the full URL string
    NSString * baseURLString = [MPAPIEndpoints baseURLStringWithPath:endpointPath];

    NSMutableArray * queryItems = [NSMutableArray array];
    [parameters enumerateKeysAndObjectsUsingBlock:^(NSString * _Nonnull key, NSString * _Nonnull value, BOOL * _Nonnull stop) {
        NSString * queryItem = [self queryItemForKey:key value:value];
        [queryItems addObject:queryItem];
    }];

    NSString * queryParameters = [queryItems componentsJoinedByString:@"&"];
    NSString * url = [NSString stringWithFormat:@"%@?%@", baseURLString, queryParameters];

    return [NSURL URLWithString:url];
}

+ (NSString *)queryItemForKey:(NSString *)key value:(NSString *)value {
    if (key == nil || value == nil) {
        return nil;
    }

    NSString * encodedValue = [value mp_URLEncodedString];
    return [NSString stringWithFormat:@"%@=%@", key, encodedValue];
}

@end

@implementation MPAdServerURLBuilder (OpenEndpoint)

+ (NSURL *)conversionTrackingURLForAppID:(NSString *)appID {
    return [self openEndpointURLWithIDParameter:appID isSessionTracking:NO];
}

+ (NSURL *)sessionTrackingURL {
    NSString *bundleIdentifier = [[[NSBundle mainBundle] bundleIdentifier] mp_URLEncodedString];
    return [self openEndpointURLWithIDParameter:bundleIdentifier isSessionTracking:YES];
}

+ (NSURL *)openEndpointURLWithIDParameter:(NSString *)idParameter isSessionTracking:(BOOL)isSessionTracking {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    NSMutableDictionary * queryParameters = [NSMutableDictionary dictionary];

    // REQUIRED: IDFA
    queryParameters[kIdfaKey] = [MPIdentityProvider identifier];

    // REQUIRED: ID Parameter (used for different things depending on which URL, take from method parameter)
    queryParameters[kOpenEndpointIDKey] = idParameter;

    // REQUIRED: Server API Version
    queryParameters[kServerAPIVersionKey] = MP_SERVER_VERSION;

    // REQUIRED: SDK Version
    queryParameters[kSDKVersionKey] = MP_SDK_VERSION;

    // REQUIRED: Application Version
    queryParameters[kApplicationVersionKey] = [self URLEncodedApplicationVersion];

    // OPTIONAL: Include Session Tracking Parameter if needed
    if (isSessionTracking) {
        queryParameters[kOpenEndpointSessionTrackingKey] = @"1";
    }

    // REQUIRED: GDPR region applicable
    if (manager.isGDPRApplicable != MPBoolUnknown) {
        queryParameters[kGDPRAppliesKey] = manager.isGDPRApplicable > 0 ? @"1" : @"0";
    }

    // REQUIRED: Current consent status
    queryParameters[kCurrentConsentStatusKey] = [NSString stringFromConsentStatus:manager.currentStatus];

    // OPTIONAL: Consented versions
    queryParameters[kConsentedPrivacyPolicyVersionKey] = manager.consentedPrivacyPolicyVersion;
    queryParameters[kConsentedVendorListVersionKey] = manager.consentedVendorListVersion;

    return [self URLWithEndpointPath:MOPUB_API_PATH_OPEN queryParameters:queryParameters];
}

@end

@implementation MPAdServerURLBuilder (Consent)

#pragma mark - Consent URLs

+ (NSURL *)consentSynchronizationUrl {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    // REQUIRED: Ad unit ID for consent may be empty if the publisher
    // never initialized the SDK.
    NSMutableDictionary * queryParameters = [NSMutableDictionary dictionary];
    queryParameters[kAdUnitIdKey] = manager.adUnitIdUsedForConsent;

    // REQUIRED: SDK version, DNT, Bundle
    queryParameters[kSDKVersionKey] = MP_SDK_VERSION;
    queryParameters[kDoNotTrackIdKey] = [MPIdentityProvider advertisingTrackingEnabled] ? nil : @"1";
    queryParameters[kBundleKey] = [[[NSBundle mainBundle] bundleIdentifier] mp_URLEncodedString];

    // REQUIRED: Current consent status
    queryParameters[kCurrentConsentStatusKey] = [NSString stringFromConsentStatus:manager.currentStatus];

    // REQUIRED: GDPR region applicable
    if (manager.isGDPRApplicable != MPBoolUnknown) {
        queryParameters[kGDPRAppliesKey] = manager.isGDPRApplicable > 0 ? @"1" : @"0";
    }

    // OPTIONAL: IFA for consent, last synchronized consent status, last changed reason,
    // last changed timestamp in milliseconds
    queryParameters[kIdfaKey] = manager.ifaForConsent;
    queryParameters[kLastSynchronizedConsentStatusKey] = manager.lastSynchronizedStatus;
    queryParameters[kConsentChangedReasonKey] = manager.lastChangedReason;
    queryParameters[kLastChangedMsKey] = manager.lastChangedTimestampInMilliseconds > 0 ? [NSString stringWithFormat:@"%llu", (unsigned long long)manager.lastChangedTimestampInMilliseconds] : nil;

    // OPTIONAL: Consented versions
    queryParameters[kConsentedPrivacyPolicyVersionKey] = manager.consentedPrivacyPolicyVersion;
    queryParameters[kConsentedVendorListVersionKey] = manager.consentedVendorListVersion;
    queryParameters[kCachedIabVendorListHashKey] = manager.iabVendorListHash;

    // OPTIONAL: Server extras
    queryParameters[kExtrasKey] = manager.extras;

    return [self URLWithEndpointPath:MOPUB_API_PATH_CONSENT_SYNC queryParameters:queryParameters];
}

+ (NSURL *)consentDialogURL {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    // REQUIRED: Ad unit ID for consent may be empty if the publisher
    // never initialized the SDK.
    NSMutableDictionary * queryParameters = [NSMutableDictionary dictionary];
    queryParameters[kAdUnitIdKey] = manager.adUnitIdUsedForConsent;

    // REQUIRED: SDK version, DNT, Bundle, Language
    queryParameters[kSDKVersionKey] = MP_SDK_VERSION;
    queryParameters[kDoNotTrackIdKey] = [MPIdentityProvider advertisingTrackingEnabled] ? nil : @"1";
    queryParameters[kBundleKey] = [[[NSBundle mainBundle] bundleIdentifier] mp_URLEncodedString];
    queryParameters[kLanguageKey] = manager.currentLanguageCode;

    return [self URLWithEndpointPath:MOPUB_API_PATH_CONSENT_DIALOG queryParameters:queryParameters];
}

@end

