//
//  MPAdServerKeys.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

#pragma mark - Ad Server All URL Request Keys
extern NSString * const kAdServerIDKey;
extern NSString * const kServerAPIVersionKey;
extern NSString * const kApplicationVersionKey;
extern NSString * const kIdfaKey;
extern NSString * const kMoPubIDKey;
extern NSString * const kBundleKey;
extern NSString * const kDoNotTrackIdKey;
extern NSString * const kSDKVersionKey;
extern NSString * const kSDKEngineNameKey;
extern NSString * const kSDKEngineVersionKey;

#pragma mark - Ad Server Ad Request Endpoint Keys
extern NSString * const kOrientationKey;
extern NSString * const kScaleFactorKey;
extern NSString * const kTimeZoneKey;
extern NSString * const kIsMRAIDEnabledSDKKey;
extern NSString * const kConnectionTypeKey;
extern NSString * const kCarrierNameKey;
extern NSString * const kISOCountryCodeKey;
extern NSString * const kMobileNetworkCodeKey;
extern NSString * const kMobileCountryCodeKey;
extern NSString * const kDeviceNameKey;
extern NSString * const kDesiredAdAssetsKey;
extern NSString * const kAdSequenceKey;
extern NSString * const kScreenResolutionWidthKey;
extern NSString * const kScreenResolutionHeightKey;
extern NSString * const kAppTransportSecurityStatusKey;
extern NSString * const kViewabilityStatusKey;
extern NSString * const kKeywordsKey;
extern NSString * const kUserDataKeywordsKey;
extern NSString * const kAdvancedBiddingKey;
extern NSString * const kNetworkAdaptersKey;
extern NSString * const kLocationLatitudeLongitudeKey;
extern NSString * const kLocationHorizontalAccuracy;
extern NSString * const kLocationIsFromSDK;
extern NSString * const kLocationLastUpdatedMilliseconds;
extern NSString * const kBackoffMsKey;
extern NSString * const kBackoffReasonKey;
extern NSString * const kCreativeSafeWidthKey;
extern NSString * const kCreativeSafeHeightKey;

#pragma mark - Ad Server Response Keys
extern NSString * const kEnableDebugLogging;

#pragma mark - Open Endpoint Request Keys
extern NSString * const kOpenEndpointSessionTrackingKey;

#pragma mark - Synchronization Keys Shared With Other Endpoints
extern NSString * const kGDPRAppliesKey;
extern NSString * const kCurrentConsentStatusKey;
extern NSString * const kConsentedVendorListVersionKey;
extern NSString * const kConsentedPrivacyPolicyVersionKey;
extern NSString * const kForceGDPRAppliesKey;

#pragma mark - Synchronization Endpoint: Request Keys

extern NSString * const kLastChangedMsKey;
extern NSString * const kLastSynchronizedConsentStatusKey;
extern NSString * const kCachedIabVendorListHashKey;
extern NSString * const kForcedGDPRAppliesChangedKey;

#pragma mark - Synchronization Endpoint: Shared Keys

extern NSString * const kConsentChangedReasonKey;
extern NSString * const kExtrasKey;

#pragma mark - Synchronization Endpoint: Response Keys

extern NSString * const kForceExplicitNoKey;
extern NSString * const kInvalidateConsentKey;
extern NSString * const kReacquireConsentKey;
extern NSString * const kIsWhitelistedKey;
extern NSString * const kIsGDPRRegionKey;
extern NSString * const kVendorListUrlKey;
extern NSString * const kVendorListVersionKey;
extern NSString * const kPrivacyPolicyUrlKey;
extern NSString * const kPrivacyPolicyVersionKey;
extern NSString * const kIabVendorListKey;
extern NSString * const kIabVendorListHashKey;
extern NSString * const kSyncFrequencyKey;

#pragma mark - Consent Dialog Endpoint: Request Keys

extern NSString * const kLanguageKey;

#pragma mark - Consent Dialog Endpoint: Response Keys

extern NSString * const kDialogHTMLKey;

#pragma mark - Rewarded Keys

extern NSString * const kCustomerIdKey;
extern NSString * const kRewardedCurrencyNameKey;
extern NSString * const kRewardedCurrencyAmountKey;
extern NSString * const kRewardedCustomEventNameKey;
extern NSString * const kRewardedCustomDataKey;

#pragma mark - Impression Level Revenue Data Keys

extern NSString * const kImpressionDataImpressionIDKey;
extern NSString * const kImpressionDataAdUnitIDKey;
extern NSString * const kImpressionDataAdUnitNameKey;
extern NSString * const kImpressionDataAdUnitFormatKey;
extern NSString * const kImpressionDataAdGroupIDKey;
extern NSString * const kImpressionDataAdGroupNameKey;
extern NSString * const kImpressionDataAdGroupTypeKey;
extern NSString * const kImpressionDataAdGroupPriorityKey;
extern NSString * const kImpressionDataCurrencyKey;
extern NSString * const kImpressionDataCountryKey;
extern NSString * const kImpressionDataNetworkNameKey;
extern NSString * const kImpressionDataNetworkPlacementIDKey;
extern NSString * const kImpressionDataAppVersionKey;
extern NSString * const kImpressionDataPublisherRevenueKey;
extern NSString * const kImpressionDataPrecisionKey;
