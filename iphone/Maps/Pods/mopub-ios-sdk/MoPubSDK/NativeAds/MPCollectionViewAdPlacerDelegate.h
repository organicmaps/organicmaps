//
//  MPCollectionViewAdPlacerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubAdPlacer.h"

@class MPCollectionViewAdPlacer;

@protocol MPCollectionViewAdPlacerDelegate <MPMoPubAdPlacerDelegate>

@optional

/*
 * This method is called when a native ad, placed by the collection view ad placer, will present a modal view controller.
 *
 * @param placer The collection view ad placer that contains the ad displaying the modal.
 */
- (void)nativeAdWillPresentModalForCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the collection view ad placer, did dismiss its modal view controller.
 *
 * @param placer The collection view ad placer that contains the ad that dismissed the modal.
 */
- (void)nativeAdDidDismissModalForCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the collection view ad placer, will cause the app to background due to user interaction with the ad.
 *
 * @param placer The collection view ad placer that contains the ad causing the app to background.
 */
- (void)nativeAdWillLeaveApplicationFromCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

@end
