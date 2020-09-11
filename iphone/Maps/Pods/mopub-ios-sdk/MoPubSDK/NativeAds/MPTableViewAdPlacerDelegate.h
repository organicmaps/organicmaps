//
//  MPTableViewAdPlacerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubAdPlacer.h"

@class MPTableViewAdPlacer;

@protocol MPTableViewAdPlacerDelegate <MPMoPubAdPlacerDelegate>

@optional

/*
 * This method is called when a native ad, placed by the table view ad placer, will present a modal view controller.
 *
 * @param placer The table view ad placer that contains the ad displaying the modal.
 */
- (void)nativeAdWillPresentModalForTableViewAdPlacer:(MPTableViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the table view ad placer, did dismiss its modal view controller.
 *
 * @param placer The table view ad placer that contains the ad that dismissed the modal.
 */
- (void)nativeAdDidDismissModalForTableViewAdPlacer:(MPTableViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the table view ad placer, will cause the app to background due to user interaction with the ad.
 *
 * @param placer The table view ad placer that contains the ad causing the app to background.
 */
- (void)nativeAdWillLeaveApplicationFromTableViewAdPlacer:(MPTableViewAdPlacer *)placer;

@end
