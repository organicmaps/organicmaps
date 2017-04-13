//
//  MRController.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "MRConstants.h"
#import "MPMRAIDInterstitialViewController.h"

@protocol MRControllerDelegate;
@class MPAdConfiguration;
@class CLLocation;

/**
 * The `MRController` class is used to load and interact with MRAID ads.
 * It contains two MRAID ad views and uses a separate `MRBridge` to
 * communicate to each view. `MRController` handles view-related MRAID
 * native calls such as expand(), resize(), close(), and open().
 */
@interface MRController : NSObject

@property (nonatomic, weak) id<MRControllerDelegate> delegate;

- (instancetype)initWithAdViewFrame:(CGRect)adViewFrame adPlacementType:(MRAdViewPlacementType)placementType;

- (void)loadAdWithConfiguration:(MPAdConfiguration *)configuration;
- (void)handleMRAIDInterstitialDidPresentWithViewController:(MPMRAIDInterstitialViewController *)viewController;
- (void)enableRequestHandling;
- (void)disableRequestHandling;

@end

/**
 * The `MRControllerDelegate` will relay specific events such as ad loading to
 * the object that implements the delegate. It also requires information
 * such as adUnitId, adConfiguation, and location in order to use its
 * ad alert manager.
 **/
@protocol MRControllerDelegate <NSObject>

@required

- (NSString *)adUnitId;
- (MPAdConfiguration *)adConfiguration;
- (CLLocation *)location;

// Retrieves the view controller from which modal views should be presented.
- (UIViewController *)viewControllerForPresentingModalView;

// Called when the ad is about to display modal content (thus taking over the screen).
- (void)appShouldSuspendForAd:(UIView *)adView;

// Called when the ad has dismissed any modal content (removing any on-screen takeovers).
- (void)appShouldResumeFromAd:(UIView *)adView;

@optional

// Called when the ad loads successfully.
- (void)adDidLoad:(UIView *)adView;

// Called when the ad fails to load.
- (void)adDidFailToLoad:(UIView *)adView;

// Called just before the ad closes.
- (void)adWillClose:(UIView *)adView;

// Called just after the ad has closed.
- (void)adDidClose:(UIView *)adView;

// Called after the rewarded video finishes playing
- (void)rewardedVideoEnded;

@end
