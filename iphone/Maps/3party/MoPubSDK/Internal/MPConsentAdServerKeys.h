//
//  MPConsentAdServerKeys.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

#pragma mark - Synchronization Endpoint: Request Keys

extern NSString * const kIdfaKey;
extern NSString * const kAdUnitIdKey;
extern NSString * const kBundleKey;
extern NSString * const kDoNotTrackIdKey;
extern NSString * const kLastChangedMsKey;
extern NSString * const kLastSynchronizedConsentStatusKey;
extern NSString * const kCurrentConsentStatusKey;
extern NSString * const kConsentedVendorListVersionKey;
extern NSString * const kConsentedPrivacyPolicyVersionKey;
extern NSString * const kCachedIabVendorListHashKey;
extern NSString * const kGDPRAppliesKey;

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
extern NSString * const kSDKVersionKey;

#pragma mark - Consent Dialog Endpoint: Response Keys

extern NSString * const kDialogHTMLKey;
