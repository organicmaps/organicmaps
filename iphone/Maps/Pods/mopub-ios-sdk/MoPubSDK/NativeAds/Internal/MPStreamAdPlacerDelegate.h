//
//  MPStreamAdPlacerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubAdPlacer.h"

@class MPStreamAdPlacer;

@protocol MPStreamAdPlacerDelegate <MPMoPubAdPlacerDelegate>

@optional
- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didLoadAdAtIndexPath:(NSIndexPath *)indexPath;
- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didRemoveAdsAtIndexPaths:(NSArray *)indexPaths;

/*
 * This method is called when a native ad, placed by the stream ad placer, will present a modal view controller.
 *
 * @param placer The stream ad placer that contains the ad displaying the modal.
 */
- (void)nativeAdWillPresentModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

/*
 * This method is called when a native ad, placed by the stream ad placer, did dismiss its modal view controller.
 *
 * @param placer The stream ad placer that contains the ad that dismissed the modal.
 */
- (void)nativeAdDidDismissModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

/*
 * This method is called when a native ad, placed by the stream ad placer, will cause the app to background due to user interaction with the ad.
 *
 * @param placer The stream ad placer that contains the ad causing the app to background.
 */
- (void)nativeAdWillLeaveApplicationFromStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

@end
