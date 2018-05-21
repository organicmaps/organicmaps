//
//  MPConsentAdServerKeys.m
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import "MPConsentAdServerKeys.h"

#pragma mark - Synchronization Endpoint: Request Keys

NSString * const kIdfaKey                          = @"udid";
NSString * const kAdUnitIdKey                      = @"id";
NSString * const kBundleKey                        = @"bundle";
NSString * const kDoNotTrackIdKey                  = @"dnt";
NSString * const kLastChangedMsKey                 = @"last_changed_ms";
NSString * const kLastSynchronizedConsentStatusKey = @"last_consent_status";
NSString * const kCurrentConsentStatusKey          = @"current_consent_status";
NSString * const kConsentedVendorListVersionKey    = @"consented_vendor_list_version";
NSString * const kConsentedPrivacyPolicyVersionKey = @"consented_privacy_policy_version";
NSString * const kCachedIabVendorListHashKey       = @"cached_vendor_list_iab_hash";
NSString * const kGDPRAppliesKey                   = @"gdpr_applies";

#pragma mark - Synchronization Endpoint: Shared Keys

NSString * const kConsentChangedReasonKey          = @"consent_change_reason";
NSString * const kExtrasKey                        = @"extras";

#pragma mark - Synchronization Endpoint: Response Keys

NSString * const kForceExplicitNoKey               = @"force_explicit_no";
NSString * const kInvalidateConsentKey             = @"invalidate_consent";
NSString * const kReacquireConsentKey              = @"reacquire_consent";
NSString * const kIsWhitelistedKey                 = @"is_whitelisted";
NSString * const kIsGDPRRegionKey                  = @"is_gdpr_region";
NSString * const kVendorListUrlKey                 = @"current_vendor_list_link";
NSString * const kVendorListVersionKey             = @"current_vendor_list_version";
NSString * const kPrivacyPolicyUrlKey              = @"current_privacy_policy_link";
NSString * const kPrivacyPolicyVersionKey          = @"current_privacy_policy_version";
NSString * const kIabVendorListKey                 = @"current_vendor_list_iab_format";
NSString * const kIabVendorListHashKey             = @"current_vendor_list_iab_hash";
NSString * const kSyncFrequencyKey                 = @"call_again_after_secs";

#pragma mark - Consent Dialog Endpoint: Request Keys

NSString * const kLanguageKey                      = @"language";
NSString * const kSDKVersionKey                    = @"nv";

#pragma mark - Consent Dialog Endpoint: Response Keys

NSString * const kDialogHTMLKey                    = @"dialog_html";
