//
//  MPConsentChangedNotification.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 Notification fire whenever the current consent status has changed. The payload
 will be included in the @NSNotification.userInfo dictionary.
 */
extern NSString * const kMPConsentChangedNotification;

/**
 The new consent state; represented as a @c MPConsentStatus value wrapped
 in a @c NSNumber.
 */
extern NSString * const kMPConsentChangedInfoNewConsentStatusKey;

/**
 The previous consent state; represented as a @c MPConsentStatus value wrapped
 in a @c NSNumber.
 */
extern NSString * const kMPConsentChangedInfoPreviousConsentStatusKey;

/**
 Boolean flag indicating that it is okay to collection any personally
 identifiable information; represented as a @c BOOL value wrapped in
 a @c NSNumber.
 */
extern NSString * const kMPConsentChangedInfoCanCollectPersonalInfoKey;
