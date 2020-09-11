//
//  MPInterstitialAdManagerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPInterstitialAdManager;
@class MPInterstitialAdController;
@class MPImpressionData;
@class CLLocation;

@protocol MPInterstitialAdManagerDelegate <NSObject>

- (MPInterstitialAdController *)interstitialAdController;
- (CLLocation *)location;
- (NSString *)adUnitId;
- (id)interstitialDelegate;
- (void)managerDidLoadInterstitial:(MPInterstitialAdManager *)manager;
- (void)manager:(MPInterstitialAdManager *)manager
didFailToLoadInterstitialWithError:(NSError *)error;
- (void)managerWillPresentInterstitial:(MPInterstitialAdManager *)manager;
- (void)managerDidPresentInterstitial:(MPInterstitialAdManager *)manager;
- (void)managerWillDismissInterstitial:(MPInterstitialAdManager *)manager;
- (void)managerDidDismissInterstitial:(MPInterstitialAdManager *)manager;
- (void)managerDidExpireInterstitial:(MPInterstitialAdManager *)manager;
- (void)interstitialAdManager:(MPInterstitialAdManager *)manager didReceiveImpressionEventWithImpressionData:(MPImpressionData *)impressionData;
- (void)managerDidReceiveTapEventFromInterstitial:(MPInterstitialAdManager *)manager;

@end
