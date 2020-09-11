//
//  MPImpressionTrackedNotification.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPImpressionTrackedNotification.h"

#pragma mark - NSNotification Name

NSString * const kMPImpressionTrackedNotification = @"com.mopub.impression-notification.impression-received";

#pragma mark - NSNotification userInfo Keys

NSString * const kMPImpressionTrackedInfoImpressionDataKey = @"com.mopub.impression-notification.userinfo.impression-data";
NSString * const kMPImpressionTrackedInfoAdUnitIDKey = @"com.mopub.impression-notification.userinfo.adunit-id";
