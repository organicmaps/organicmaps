//
//  MoPub+Utility.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MoPub.h"

NS_ASSUME_NONNULL_BEGIN

@interface MoPub (Utility)

/**
 This is a simplified version of @c [MoPub openURL:options:completion:], which provides empty @c options
 dictionary and nil @c completion.

 @param url A URL (Universal Resource Locator).
 */
+ (void)openURL:(NSURL*)url;

/**
 This is a wrapper method that picks the correct version of @c [UIApplication  openURL:] (versus
 @c [UIApplication  openURL:options:completionHandler:]) base the iOS target.

 @param url A URL (Universal Resource Locator).
 @param options A dictionary of options to use when opening the URL.
 @param completion The block to execute with the results.
 */
+ (void)openURL:(NSURL*)url
        options:(NSDictionary<UIApplicationOpenExternalURLOptionsKey, id> *)options
     completion:(void (^ __nullable)(BOOL success))completion;

/**
 This method sends an impression @c NSNotification.

 @param ad the ad from which to send the notification, or @c nil
 @param adUnitID the adunit ID of the ad that sent the notification
 @param impressionData the impression data associated with the ad, or nil if no impression data
 */
+ (void)sendImpressionNotificationFromAd:(id _Nullable)ad
                                adUnitID:(NSString *)adUnitID
                          impressionData:(MPImpressionData * _Nullable)impressionData;

/**
 This method sends an impression @c NSNotification and notifies the @c ad's delegate of the impression.

 @param ad the ad from which to send the notification
 @param adUnitID the adunit ID of the ad that sent the notification
 @param impressionData the impression data associated with the ad, or nil if no impression data
 */
+ (void)sendImpressionDelegateAndNotificationFromAd:(id<MPMoPubAd>)ad
                                           adUnitID:(NSString *)adUnitID
                                     impressionData:(MPImpressionData * _Nullable)impressionData;

@end

NS_ASSUME_NONNULL_END
