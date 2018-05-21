//
//  MPConsentError.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

static NSString * const kConsentErrorDomain = @"com.mopub.mopub-ios-sdk.consent";

/**
 Error codes related to consent management.
 */
typedef NS_ENUM(NSUInteger, MPConsentErrorCode) {
    MPConsentErrorCodeLimitAdTrackingEnabled = 1,
    MPConsentErrorCodeFailedToParseSynchronizationResponse,
};
