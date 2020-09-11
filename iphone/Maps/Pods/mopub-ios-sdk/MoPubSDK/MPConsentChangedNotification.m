//
//  MPConsentChangedNotification.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPConsentChangedNotification.h"

#pragma mark - NSNotification Name

NSString * const kMPConsentChangedNotification = @"com.mopub.mopub-ios-sdk.notification.consent.changed";

#pragma mark - NSNotification userInfo Keys

NSString * const kMPConsentChangedInfoNewConsentStatusKey = @"newConsentStatus";
NSString * const kMPConsentChangedInfoPreviousConsentStatusKey = @"previousConsentStatus";
NSString * const kMPConsentChangedInfoCanCollectPersonalInfoKey = @"canCollectPersonalInfo";
