//
//  MoPub+Utility.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MoPub+Utility.h"

@implementation MoPub (Utility)

+ (void)openURL:(NSURL*)url {
    [self openURL:url options:@{} completion:nil];
}

+ (void)openURL:(NSURL*)url
        options:(NSDictionary<UIApplicationOpenExternalURLOptionsKey, id> *)options
     completion:(void (^ __nullable)(BOOL success))completion {
    if (@available(iOS 10, *)) {
        [[UIApplication sharedApplication] openURL:url options:options completionHandler:completion];
    } else {
        completion([[UIApplication sharedApplication] openURL:url]);
    }
}

+ (void)sendImpressionNotificationFromAd:(id)ad
                                adUnitID:(NSString *)adUnitID
                          impressionData:(MPImpressionData * _Nullable)impressionData {
    // This dictionary must always contain the adunit ID but may or may not include @c impressionData depending on if it's @c nil.
    // If adding keys and objects in the future, put them above @c impressionData to avoid being skipped in the case of nil data.
    NSDictionary * userInfo = [NSDictionary dictionaryWithObjectsAndKeys:adUnitID,
                               kMPImpressionTrackedInfoAdUnitIDKey,
                               impressionData,
                               kMPImpressionTrackedInfoImpressionDataKey,
                               nil];
    [[NSNotificationCenter defaultCenter] postNotificationName:kMPImpressionTrackedNotification
                                                        object:ad
                                                      userInfo:userInfo];
}

+ (void)sendImpressionDelegateAndNotificationFromAd:(id<MPMoPubAd>)ad
                                           adUnitID:(NSString *)adUnitID
                                     impressionData:(MPImpressionData * _Nullable)impressionData {
    [self sendImpressionNotificationFromAd:ad
                                  adUnitID:adUnitID
                            impressionData:impressionData];

    if ([ad.delegate respondsToSelector:@selector(mopubAd:didTrackImpressionWithImpressionData:)]) {
        [ad.delegate mopubAd:ad didTrackImpressionWithImpressionData:impressionData];
    }
}

@end
