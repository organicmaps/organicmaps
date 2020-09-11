//
//  MPConsentError.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

static NSString * const kConsentErrorDomain = @"com.mopub.mopub-ios-sdk.consent";

/**
 Error codes related to consent management.
 */
typedef NS_ENUM(NSUInteger, MPConsentErrorCode) {
    MPConsentErrorCodeLimitAdTrackingEnabled = 1,
    MPConsentErrorCodeFailedToParseSynchronizationResponse,
    MPConsentErrorCodeGDPRIsNotApplicable,
};
