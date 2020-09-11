//
//  MPConsentChangedNotification.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

/**
 Notification fired whenever the current consent status has changed. The payload
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
