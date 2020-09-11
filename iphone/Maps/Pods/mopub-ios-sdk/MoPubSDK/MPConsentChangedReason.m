//
//  MPConsentChangedReason.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPConsentChangedReason.h"

// Human readable descriptions of why consent changed.
NSString * const kConsentedChangedReasonGranted = @"Consent was explicitly granted by the user";
NSString * const kConsentedChangedReasonWhitelistGranted = @"Consent was explicitly granted by a whitelisted publisher";
NSString * const kConsentedChangedReasonPotentialWhitelist = @"Consent was explicitly granted by a publisher who is not whitelisted";
NSString * const kConsentedChangedReasonDenied = @"Consent was explicitly denied by the user";
NSString * const kConsentedChangedReasonPublisherDenied = @"Consent was explicitly denied by the publisher";
NSString * const kConsentedChangedReasonDoNotTrackEnabled = @"Limit ad tracking was enabled and consent implicitly denied by the user";
NSString * const kConsentedChangedReasonDoNotTrackDisabled = @"Limit ad tracking was disabled";
NSString * const kConsentedChangedReasonDoNotTrackDisabledNeedConsent = @"Consent needs to be reacquired because the user disabled limit ad tracking";
NSString * const kConsentedChangedReasonIfaChanged = @"Consent needs to be reacquired because the IFA has changed";
NSString * const kConsentedChangedReasonPrivacyPolicyChange = @"Consent needs to be reacquired because the privacy policy has changed";
NSString * const kConsentedChangedReasonVendorListChange = @"Consent needs to be reacquired because the vendor list has changed";
NSString * const kConsentedChangedReasonIabVendorListChange = @"Consent needs to be reacquired because the IAB vendor list has changed";
NSString * const kConsentedChangedReasonServerDeniedConsent = @"Consent was revoked by the server";
NSString * const kConsentedChangedReasonServerForceInvalidate = @"Server requires that consent needs to be reacquired";
