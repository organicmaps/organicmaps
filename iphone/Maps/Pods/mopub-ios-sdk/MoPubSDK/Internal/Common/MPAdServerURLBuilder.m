//
//  MPAdServerURLBuilder.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdServerURLBuilder.h"

#import <CoreLocation/CoreLocation.h>

#import "MPAdServerKeys.h"
#import "MPAPIEndpoints.h"
#import "MPConsentManager.h"
#import "MPConstants.h"
#import "MPCoreInstanceProvider+MRAID.h"
#import "MPDeviceInformation.h"
#import "MPError.h"
#import "MPGeolocationProvider.h"
#import "MPGlobal.h"
#import "MPIdentityProvider.h"
#import "MPLogging.h"
#import "MPMediationManager.h"
#import "MPRateLimitManager.h"
#import "MPReachabilityManager.h"
#import "MPViewabilityTracker.h"
#import "NSString+MPAdditions.h"
#import "NSString+MPConsentStatus.h"

static NSString * const kMoPubInterfaceOrientationPortrait = @"p";
static NSString * const kMoPubInterfaceOrientationLandscape = @"l";
static NSInteger const kAdSequenceNone = -1;

@interface MPAdServerURLBuilder ()

/**
 * Builds an NSMutableDictionary with all generic URL parameters and their values. The `id` URL parameter is gathered from
 * the `idParameter` method parameter because it has different uses depending on the URL. This base set of parameters
 * includes the following:
 * - ID Parameter (`id`)
 * - Server API Version Used (`v`)
 * - SDK Version (`nv`)
 * - Application Version (`av`)
 * - GDPR Region Applicable (`gdpr_applies`)
 * - Current Consent Status (`current_consent_status`)
 * - Limit Ad Tracking Status (`dnt`)
 * - Bundle Identifier (`bundle`)
 * - IF AVAILABLE: Consented Privacy Policy Version (`consented_privacy_policy_version`)
 * - IF AVAILABLE: Consented Vendor List Version (`consented_vendor_list_version`)
 */
+ (NSMutableDictionary *)baseParametersDictionaryWithIDParameter:(NSString *)idParameter;

/**
 * Builds an NSMutableDictionary with all generic URL parameters above and their values, but with the addition of IDFA.
 * If @c usingIDFAForConsent is @c YES, the IDFA will be gathered from MPConsentManager (which may be nil without
 * consent). Otherwise, the IDFA will be taken from MPIdentityProvider, which will always have a value, but may be
 * MoPub's value.
 */
+ (NSMutableDictionary *)baseParametersDictionaryWithIDFAUsingIDFAForConsent:(BOOL)usingIDFAForConsent
                                                             withIDParameter:(NSString *)idParameter;

@end

@implementation MPAdServerURLBuilder

#pragma mark - Static Properties

static MPEngineInfo * _engineInfo = nil;

+ (MPEngineInfo *)engineInformation {
    return _engineInfo;
}

+ (void)setEngineInformation:(MPEngineInfo *)engineInformation {
    _engineInfo = engineInformation;
}

#pragma mark - URL Building

+ (MPURL *)URLWithEndpointPath:(NSString *)endpointPath postData:(NSDictionary *)parameters {
    // Build the full URL string
    NSURLComponents * components = [MPAPIEndpoints baseURLComponentsWithPath:endpointPath];
    return [MPURL URLWithComponents:components postData:parameters];
}

+ (NSMutableDictionary *)baseParametersDictionaryWithIDParameter:(NSString *)idParameter {
    MPConsentManager * manager = MPConsentManager.sharedManager;
    NSMutableDictionary * queryParameters = [NSMutableDictionary dictionary];

    // REQUIRED: ID Parameter (used for different things depending on which URL, take from method parameter)
    queryParameters[kAdServerIDKey] = idParameter;

    // REQUIRED: Server API Version
    queryParameters[kServerAPIVersionKey] = MP_SERVER_VERSION;

    // REQUIRED: SDK Version
    queryParameters[kSDKVersionKey] = MP_SDK_VERSION;

    // REQUIRED: SDK Engine Information
    queryParameters[kSDKEngineNameKey] = [self engineNameValue];
    queryParameters[kSDKEngineVersionKey] = [self engineVersionValue];

    // REQUIRED: Application Version
    queryParameters[kApplicationVersionKey] = [self applicationVersion];

    // REQUIRED: GDPR region applicable
    if (manager.isGDPRApplicable != MPBoolUnknown) {
        queryParameters[kGDPRAppliesKey] = manager.isGDPRApplicable > 0 ? @"1" : @"0";
    }

    // REQUIRED: GDPR applicable was forced
    queryParameters[kForceGDPRAppliesKey] = manager.forceIsGDPRApplicable ? @"1" : @"0";

    // REQUIRED: Current consent status
    queryParameters[kCurrentConsentStatusKey] = [NSString stringFromConsentStatus:manager.currentStatus];

    // REQUIRED: DNT, Bundle
    queryParameters[kDoNotTrackIdKey] = [MPIdentityProvider advertisingTrackingEnabled] ? nil : @"1";
    queryParameters[kBundleKey] = [[NSBundle mainBundle] bundleIdentifier];

    // REQUIRED: MoPub ID
    // After user consented IDFA access, UDID uses IDFA and thus different from MoPub ID.
    // Otherwise, UDID is the same as MoPub ID.
    queryParameters[kMoPubIDKey] = [MPIdentityProvider unobfuscatedMoPubIdentifier];

    // OPTIONAL: Consented versions
    queryParameters[kConsentedPrivacyPolicyVersionKey] = manager.consentedPrivacyPolicyVersion;
    queryParameters[kConsentedVendorListVersionKey] = manager.consentedVendorListVersion;

    return queryParameters;
}

+ (NSMutableDictionary *)baseParametersDictionaryWithIDFAUsingIDFAForConsent:(BOOL)usingIDFAForConsent
                                                             withIDParameter:(NSString *)idParameter {
    MPConsentManager * manager = MPConsentManager.sharedManager;
    NSMutableDictionary * queryParameters = [self baseParametersDictionaryWithIDParameter:idParameter];

    // OPTIONAL: IDFA if available
    if (usingIDFAForConsent) {
        queryParameters[kIdfaKey] = manager.ifaForConsent;
    } else {
        queryParameters[kIdfaKey] = [MPIdentityProvider identifier];
    }

    return queryParameters;
}

+ (NSString *)applicationVersion {
    static NSString * gApplicationVersion;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        gApplicationVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    });

    return gApplicationVersion;
}

+ (NSString *)engineNameValue {
    return self.engineInformation.name;
}

+ (NSString *)engineVersionValue {
    return self.engineInformation.version;
}

@end

@implementation MPAdServerURLBuilder (Ad)

+ (MPURL *)URLWithAdUnitID:(NSString *)adUnitID
                 targeting:(MPAdTargeting *)targeting
{
    return [self URLWithAdUnitID:adUnitID
                       targeting:targeting
                   desiredAssets:nil
                     viewability:YES];
}

+ (MPURL *)URLWithAdUnitID:(NSString *)adUnitID
                 targeting:(MPAdTargeting *)targeting
             desiredAssets:(NSArray *)assets
               viewability:(BOOL)viewability
{


    return [self URLWithAdUnitID:adUnitID
                       targeting:targeting
                   desiredAssets:assets
                      adSequence:kAdSequenceNone
                     viewability:viewability];
}

+ (MPURL *)URLWithAdUnitID:(NSString *)adUnitID
                 targeting:(MPAdTargeting *)targeting
             desiredAssets:(NSArray *)assets
                adSequence:(NSInteger)adSequence
               viewability:(BOOL)viewability
{
    NSMutableDictionary * queryParams = [self baseParametersDictionaryWithIDFAUsingIDFAForConsent:NO
                                                                                  withIDParameter:adUnitID];

    queryParams[kOrientationKey]                = [self orientationValue];
    queryParams[kScaleFactorKey]                = [self scaleFactorValue];
    queryParams[kTimeZoneKey]                   = [self timeZoneValue];
    queryParams[kIsMRAIDEnabledSDKKey]          = [self isMRAIDEnabledSDKValue];
    queryParams[kConnectionTypeKey]             = [self connectionTypeValue];
    queryParams[kCarrierNameKey]                = MPDeviceInformation.carrierName;
    queryParams[kISOCountryCodeKey]             = MPDeviceInformation.isoCountryCode;
    queryParams[kMobileNetworkCodeKey]          = MPDeviceInformation.mobileNetworkCode;
    queryParams[kMobileCountryCodeKey]          = MPDeviceInformation.mobileCountryCode;
    queryParams[kDeviceNameKey]                 = [self deviceNameValue];
    queryParams[kDesiredAdAssetsKey]            = [self desiredAdAssetsValue:assets];
    queryParams[kAdSequenceKey]                 = [self adSequenceValue:adSequence];
    queryParams[kScreenResolutionWidthKey]      = [self physicalScreenResolutionWidthValue];
    queryParams[kScreenResolutionHeightKey]     = [self physicalScreenResolutionHeightValue];
    queryParams[kCreativeSafeWidthKey]          = [self creativeSafeWidthValue:targeting.creativeSafeSize];
    queryParams[kCreativeSafeHeightKey]         = [self creativeSafeHeightValue:targeting.creativeSafeSize];
    queryParams[kAppTransportSecurityStatusKey] = [self appTransportSecurityStatusValue];
    queryParams[kKeywordsKey]                   = [self keywordsValue:targeting.keywords];
    queryParams[kUserDataKeywordsKey]           = [self userDataKeywordsValue:targeting.userDataKeywords];
    queryParams[kViewabilityStatusKey]          = [self viewabilityStatusValue:viewability];
    queryParams[kAdvancedBiddingKey]            = [self advancedBiddingValue];
    queryParams[kBackoffMsKey]                  = [self backoffMillisecondsValueForAdUnitID:adUnitID];
    queryParams[kBackoffReasonKey]              = [[MPRateLimitManager sharedInstance] lastRateLimitReasonForAdUnitId:adUnitID];
    [queryParams addEntriesFromDictionary:self.locationInformation];

    return [self URLWithEndpointPath:MOPUB_API_PATH_AD_REQUEST postData:queryParams];
}

+ (NSString *)orientationValue
{
    // Starting with iOS8, the orientation of the device is taken into account when
    // requesting the key window's bounds.
    CGRect appBounds = [UIApplication sharedApplication].keyWindow.bounds;
    return appBounds.size.width > appBounds.size.height ? kMoPubInterfaceOrientationLandscape : kMoPubInterfaceOrientationPortrait;
}

+ (NSString *)scaleFactorValue
{
    return [NSString stringWithFormat:@"%.1f", MPDeviceScaleFactor()];
}

+ (NSString *)timeZoneValue
{
    static NSDateFormatter *formatter;
    @synchronized(self)
    {
        if (!formatter) formatter = [[NSDateFormatter alloc] init];
    }
    [formatter setDateFormat:@"Z"];
    NSDate *today = [NSDate date];
    return [formatter stringFromDate:today];
}

+ (NSString *)isMRAIDEnabledSDKValue
{
    BOOL isMRAIDEnabled = [[MPCoreInstanceProvider sharedProvider] isMraidJavascriptAvailable] &&
                          NSClassFromString(@"MPMRAIDBannerCustomEvent") != Nil &&
                          NSClassFromString(@"MPMRAIDInterstitialCustomEvent") != Nil;
    return isMRAIDEnabled ? @"1" : nil;
}

+ (NSString *)connectionTypeValue
{
    return [NSString stringWithFormat:@"%ld", (long)MPReachabilityManager.sharedManager.currentStatus];
}

+ (NSString *)deviceNameValue
{
    NSString *deviceName = [[UIDevice currentDevice] mp_hardwareDeviceName];
    return deviceName;
}

+ (NSString *)desiredAdAssetsValue:(NSArray *)assets
{
    NSString *concatenatedAssets = [assets componentsJoinedByString:@","];
    return [concatenatedAssets length] ? concatenatedAssets : nil;
}

+ (NSString *)adSequenceValue:(NSInteger)adSequence
{
    return (adSequence >= 0) ? [NSString stringWithFormat:@"%ld", (long)adSequence] : nil;
}

+ (NSString *)physicalScreenResolutionWidthValue
{
    return [NSString stringWithFormat:@"%.0f", MPScreenResolution().width];
}

+ (NSString *)physicalScreenResolutionHeightValue
{
    return [NSString stringWithFormat:@"%.0f", MPScreenResolution().height];
}

+ (NSString *)creativeSafeWidthValue:(CGSize)creativeSafeSize
{
    CGFloat scale = MPDeviceScaleFactor();
    return [NSString stringWithFormat:@"%.0f", creativeSafeSize.width * scale];
}

+ (NSString *)creativeSafeHeightValue:(CGSize)creativeSafeSize
{
    CGFloat scale = MPDeviceScaleFactor();
    return [NSString stringWithFormat:@"%.0f", creativeSafeSize.height * scale];
}

+ (NSString *)appTransportSecurityStatusValue
{
    return [NSString stringWithFormat:@"%@", @(MPDeviceInformation.appTransportSecuritySettings)];
}

+ (NSString *)keywordsValue:(NSString *)keywords
{
    return keywords;
}

+ (NSString *)userDataKeywordsValue:(NSString *)userDataKeywords
{
    // Avoid sending user data keywords if we are not allowed to collect personal info
    if (![MPConsentManager sharedManager].canCollectPersonalInfo) {
        return nil;
    }

    return userDataKeywords;
}

+ (NSString *)viewabilityStatusValue:(BOOL)isViewabilityEnabled {
    if (!isViewabilityEnabled) {
        return nil;
    }

    return [NSString stringWithFormat:@"%d", (int)[MPViewabilityTracker enabledViewabilityVendors]];
}

+ (NSString *)advancedBiddingValue {
    // Retrieve the tokens
    NSDictionary * tokens = MPMediationManager.sharedManager.advancedBiddingTokens;
    if (tokens == nil) {
        return nil;
    }

    // Serialize the JSON dictionary into a JSON string.
    NSError * error = nil;
    NSData * jsonData = [NSJSONSerialization dataWithJSONObject:tokens options:0 error:&error];
    if (jsonData == nil) {
        NSError * jsonError = [NSError serializationOfJson:tokens failedWithError:error];
        MPLogEvent([MPLogEvent error:jsonError message:nil]);
        return nil;
    }

    return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

+ (NSString *)backoffMillisecondsValueForAdUnitID:(NSString *)adUnitID {
    NSUInteger lastRateLimitWaitTimeMilliseconds = [[MPRateLimitManager sharedInstance] lastRateLimitMillisecondsForAdUnitId:adUnitID];
    return lastRateLimitWaitTimeMilliseconds > 0 ? [NSString stringWithFormat:@"%@", @(lastRateLimitWaitTimeMilliseconds)] : nil;
}

+ (NSDictionary *)adapterInformation {
    return MPMediationManager.sharedManager.adRequestPayload;
}

+ (NSDictionary *)locationInformation {
    // Not allowed to collect location because it is PII
    if (![MPConsentManager.sharedManager canCollectPersonalInfo]) {
        return @{};
    }

    NSMutableDictionary *locationDict = [NSMutableDictionary dictionary];

    CLLocation *bestLocation = MPGeolocationProvider.sharedProvider.lastKnownLocation;
    if (bestLocation && bestLocation.horizontalAccuracy >= 0) {
        locationDict[kLocationLatitudeLongitudeKey] = [NSString stringWithFormat:@"%@,%@",
                                                       @(bestLocation.coordinate.latitude),
                                                       @(bestLocation.coordinate.longitude)];
        if (bestLocation.horizontalAccuracy) {
            locationDict[kLocationHorizontalAccuracy] = [NSString stringWithFormat:@"%@", @(bestLocation.horizontalAccuracy)];
        }

        // Only SDK-specified locations are allowed.
        locationDict[kLocationIsFromSDK] = @"1";

        NSTimeInterval locationLastUpdatedMillis = [[NSDate date] timeIntervalSinceDate:bestLocation.timestamp] * 1000.0;
        locationDict[kLocationLastUpdatedMilliseconds] = [NSString stringWithFormat:@"%.0f", locationLastUpdatedMillis];
    }

    return locationDict;
}

@end

@implementation MPAdServerURLBuilder (Open)

+ (MPURL *)conversionTrackingURLForAppID:(NSString *)appID {
    return [self openEndpointURLWithIDParameter:appID isSessionTracking:NO];
}

+ (MPURL *)sessionTrackingURL {
    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
    return [self openEndpointURLWithIDParameter:bundleIdentifier isSessionTracking:YES];
}

+ (MPURL *)openEndpointURLWithIDParameter:(NSString *)idParameter isSessionTracking:(BOOL)isSessionTracking {
    NSMutableDictionary * queryParameters = [self baseParametersDictionaryWithIDFAUsingIDFAForConsent:NO
                                                                                      withIDParameter:idParameter];

    // OPTIONAL: Include Session Tracking Parameter if needed
    if (isSessionTracking) {
        queryParameters[kOpenEndpointSessionTrackingKey] = @"1";
    }

    return [self URLWithEndpointPath:MOPUB_API_PATH_OPEN postData:queryParameters];
}

@end

@implementation MPAdServerURLBuilder (Consent)

#pragma mark - Consent URLs

+ (MPURL *)consentSynchronizationUrl {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    // REQUIRED: Ad unit ID for consent may be empty if the publisher
    // never initialized the SDK.
    NSMutableDictionary * postData = [self baseParametersDictionaryWithIDFAUsingIDFAForConsent:YES
                                                                               withIDParameter:manager.adUnitIdUsedForConsent];

    // OPTIONAL: Last synchronized consent status, last changed reason,
    // last changed timestamp in milliseconds
    postData[kLastSynchronizedConsentStatusKey] = manager.lastSynchronizedStatus;
    postData[kConsentChangedReasonKey] = manager.lastChangedReason;
    postData[kLastChangedMsKey] = manager.lastChangedTimestampInMilliseconds > 0 ? [NSString stringWithFormat:@"%llu", (unsigned long long)manager.lastChangedTimestampInMilliseconds] : nil;

    // OPTIONAL: Cached IAB Vendor List Hash Key
    postData[kCachedIabVendorListHashKey] = manager.iabVendorListHash;

    // OPTIONAL: Server extras
    postData[kExtrasKey] = manager.extras;

    // OPTIONAL: Force GDPR appliciability has changed
    postData[kForcedGDPRAppliesChangedKey] = manager.isForcedGDPRAppliesTransition ? @"1" : nil;

    return [self URLWithEndpointPath:MOPUB_API_PATH_CONSENT_SYNC postData:postData];
}

+ (MPURL *)consentDialogURL {
    MPConsentManager * manager = MPConsentManager.sharedManager;

    // REQUIRED: Ad unit ID for consent; may be empty if the publisher
    // never initialized the SDK.
    NSMutableDictionary * postData = [self baseParametersDictionaryWithIDParameter:manager.adUnitIdUsedForConsent];

    // REQUIRED: Language
    postData[kLanguageKey] = manager.currentLanguageCode;

    return [self URLWithEndpointPath:MOPUB_API_PATH_CONSENT_DIALOG postData:postData];
}

@end

@implementation MPAdServerURLBuilder (Native)

+ (MPURL *)nativePositionUrlForAdUnitId:(NSString *)adUnitId {
    // No ad unit ID
    if (adUnitId == nil) {
        return nil;
    }

    NSDictionary * queryItems = [self baseParametersDictionaryWithIDFAUsingIDFAForConsent:NO withIDParameter:adUnitId];
    return [self URLWithEndpointPath:MOPUB_API_PATH_NATIVE_POSITIONING postData:queryItems];
}

@end

@implementation MPAdServerURLBuilder (Rewarded)

+ (MPURL *)rewardedCompletionUrl:(NSString *)sourceUrl
                  withCustomerId:(NSString *)customerId
                      rewardType:(NSString *)rewardType
                    rewardAmount:(NSNumber *)rewardAmount
                 customEventName:(NSString *)customEventName
                  additionalData:(NSString *)additionalData {

    NSURLComponents * components = [NSURLComponents componentsWithString:sourceUrl];

    // Build the additional query parameters to be appended to the existing set.
    NSMutableDictionary<NSString *, NSString *> * postData = [NSMutableDictionary dictionary];

    // REQUIRED: Rewarded APIVersion
    postData[kServerAPIVersionKey] = MP_REWARDED_API_VERSION;

    // REQUIRED: SDK Version
    postData[kSDKVersionKey] = MP_SDK_VERSION;

    // OPTIONAL: Customer ID
    if (customerId != nil && customerId.length > 0) {
        postData[kCustomerIdKey] = customerId;
    }

    // OPTIONAL: Rewarded currency and amount
    if (rewardType != nil && rewardType.length > 0 && rewardAmount != nil) {
        postData[kRewardedCurrencyNameKey] = rewardType;
        postData[kRewardedCurrencyAmountKey] = [NSString stringWithFormat:@"%i", rewardAmount.intValue];
    }

    // OPTIONAL: Rewarded custom event name
    if (customEventName != nil) {
        postData[kRewardedCustomEventNameKey] = customEventName;
    }

    // OPTIONAL: Additional publisher data
    if (additionalData != nil) {
        postData[kRewardedCustomDataKey] = additionalData;
    }

    return [MPURL URLWithComponents:components postData:postData];
}

@end

