//
//  MPAdServerKeys.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdServerKeys.h"

#pragma mark - Ad Server All URL Request Keys
NSString * const kAdServerIDKey                       = @"id";
NSString * const kServerAPIVersionKey                 = @"v";
NSString * const kApplicationVersionKey               = @"av";
NSString * const kIdfaKey                             = @"udid";
NSString * const kMoPubIDKey                          = @"mid";
NSString * const kBundleKey                           = @"bundle";
NSString * const kDoNotTrackIdKey                     = @"dnt";
NSString * const kSDKVersionKey                       = @"nv";
NSString * const kSDKEngineNameKey                    = @"e_name";
NSString * const kSDKEngineVersionKey                 = @"e_ver";

#pragma mark - Ad Server Ad Request Endpoint Keys
NSString * const kOrientationKey                      = @"o";
NSString * const kScaleFactorKey                      = @"sc";
NSString * const kTimeZoneKey                         = @"z";
NSString * const kIsMRAIDEnabledSDKKey                = @"mr";
NSString * const kConnectionTypeKey                   = @"ct";
NSString * const kCarrierNameKey                      = @"cn";
NSString * const kISOCountryCodeKey                   = @"iso";
NSString * const kMobileNetworkCodeKey                = @"mnc";
NSString * const kMobileCountryCodeKey                = @"mcc";
NSString * const kDeviceNameKey                       = @"dn";
NSString * const kDesiredAdAssetsKey                  = @"assets";
NSString * const kAdSequenceKey                       = @"seq";
NSString * const kScreenResolutionWidthKey            = @"w";
NSString * const kScreenResolutionHeightKey           = @"h";
NSString * const kAppTransportSecurityStatusKey       = @"ats";
NSString * const kViewabilityStatusKey                = @"vv";
NSString * const kKeywordsKey                         = @"q";
NSString * const kUserDataKeywordsKey                 = @"user_data_q";
NSString * const kAdvancedBiddingKey                  = @"abt";
NSString * const kNetworkAdaptersKey                  = @"adapters";
NSString * const kLocationLatitudeLongitudeKey        = @"ll";
NSString * const kLocationHorizontalAccuracy          = @"lla";
NSString * const kLocationIsFromSDK                   = @"llsdk";
NSString * const kLocationLastUpdatedMilliseconds     = @"llf";
NSString * const kBackoffMsKey                        = @"backoff_ms";
NSString * const kBackoffReasonKey                    = @"backoff_reason";
NSString * const kCreativeSafeWidthKey                = @"cw";
NSString * const kCreativeSafeHeightKey               = @"ch";

#pragma mark - Ad Server Response Keys
NSString * const kEnableDebugLogging                  = @"enable_debug_logging";

#pragma mark - Open Endpoint Request Keys
NSString * const kOpenEndpointSessionTrackingKey      = @"st";

#pragma mark - Synchronization Keys Shared With Other Endpoints
NSString * const kGDPRAppliesKey                      = @"gdpr_applies";
NSString * const kCurrentConsentStatusKey             = @"current_consent_status";
NSString * const kConsentedVendorListVersionKey       = @"consented_vendor_list_version";
NSString * const kConsentedPrivacyPolicyVersionKey    = @"consented_privacy_policy_version";
NSString * const kForceGDPRAppliesKey                 = @"force_gdpr_applies";

#pragma mark - Synchronization Endpoint: Request Keys

NSString * const kLastChangedMsKey                    = @"last_changed_ms";
NSString * const kLastSynchronizedConsentStatusKey    = @"last_consent_status";
NSString * const kCachedIabVendorListHashKey          = @"cached_vendor_list_iab_hash";
NSString * const kForcedGDPRAppliesChangedKey         = @"force_gdpr_applies_changed";

#pragma mark - Synchronization Endpoint: Shared Keys

NSString * const kConsentChangedReasonKey             = @"consent_change_reason";
NSString * const kExtrasKey                           = @"extras";

#pragma mark - Synchronization Endpoint: Response Keys

NSString * const kForceExplicitNoKey                  = @"force_explicit_no";
NSString * const kInvalidateConsentKey                = @"invalidate_consent";
NSString * const kReacquireConsentKey                 = @"reacquire_consent";
NSString * const kIsWhitelistedKey                    = @"is_whitelisted";
NSString * const kIsGDPRRegionKey                     = @"is_gdpr_region";
NSString * const kVendorListUrlKey                    = @"current_vendor_list_link";
NSString * const kVendorListVersionKey                = @"current_vendor_list_version";
NSString * const kPrivacyPolicyUrlKey                 = @"current_privacy_policy_link";
NSString * const kPrivacyPolicyVersionKey             = @"current_privacy_policy_version";
NSString * const kIabVendorListKey                    = @"current_vendor_list_iab_format";
NSString * const kIabVendorListHashKey                = @"current_vendor_list_iab_hash";
NSString * const kSyncFrequencyKey                    = @"call_again_after_secs";

#pragma mark - Consent Dialog Endpoint: Request Keys

NSString * const kLanguageKey                         = @"language";

#pragma mark - Consent Dialog Endpoint: Response Keys

NSString * const kDialogHTMLKey                       = @"dialog_html";

#pragma mark - Rewarded Keys

NSString * const kCustomerIdKey                       = @"customer_id";
NSString * const kRewardedCurrencyNameKey             = @"rcn";
NSString * const kRewardedCurrencyAmountKey           = @"rca";
NSString * const kRewardedCustomEventNameKey          = @"cec";
NSString * const kRewardedCustomDataKey               = @"rcd";

#pragma mark - Impression Level Revenue Data Keys

NSString * const kImpressionDataImpressionIDKey       = @"id";
NSString * const kImpressionDataAdUnitIDKey           = @"adunit_id";
NSString * const kImpressionDataAdUnitNameKey         = @"adunit_name";
NSString * const kImpressionDataAdUnitFormatKey       = @"adunit_format";
NSString * const kImpressionDataAdGroupIDKey          = @"adgroup_id";
NSString * const kImpressionDataAdGroupNameKey        = @"adgroup_name";
NSString * const kImpressionDataAdGroupTypeKey        = @"adgroup_type";
NSString * const kImpressionDataAdGroupPriorityKey    = @"adgroup_priority";
NSString * const kImpressionDataCurrencyKey           = @"currency";
NSString * const kImpressionDataCountryKey            = @"country";
NSString * const kImpressionDataNetworkNameKey        = @"network_name";
NSString * const kImpressionDataNetworkPlacementIDKey = @"network_placement_id";
NSString * const kImpressionDataAppVersionKey         = @"app_version";
NSString * const kImpressionDataPublisherRevenueKey   = @"publisher_revenue";
NSString * const kImpressionDataPrecisionKey          = @"precision";
