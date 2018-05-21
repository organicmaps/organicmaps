//
//  MPConsentChangedNotification.m
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import "MPConsentChangedNotification.h"

#pragma mark - NSNotification Name

NSString * const kMPConsentChangedNotification = @"com.mopub.mopub-ios-sdk.notification.consent.changed";

#pragma mark - NSNotification userInfo Keys

NSString * const kMPConsentChangedInfoNewConsentStatusKey = @"newConsentStatus";
NSString * const kMPConsentChangedInfoPreviousConsentStatusKey = @"previousConsentStatus";
NSString * const kMPConsentChangedInfoCanCollectPersonalInfoKey = @"canCollectPersonalInfo";
