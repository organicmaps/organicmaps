//
//  MPConsentStatus.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 Personally identifiable information (PII) consent status.
 PII is allowed to be collected only if @c MPConsentStatus is @c MPConsentStatusConsented.
 In all other cases, PII is not allowed to be collected.
 */
typedef NS_ENUM(NSInteger, MPConsentStatus) {
    /**
     Status is unknown. Either the status is currently updating or
     @c initializeSdkWithConfiguration:completion: has not been called.
     */
    MPConsentStatusUnknown = 0,

    /**
     Consent is denied.
     */
    MPConsentStatusDenied,

    /**
     Advertiser tracking is disabled.
     */
    MPConsentStatusDoNotTrack,

    /**
     A potentially whitelisted publisher has attempted to grant consent on
     the user's behalf.
     */
    MPConsentStatusPotentialWhitelist,

    /**
     Consented.
     */
    MPConsentStatusConsented
};
