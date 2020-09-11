//
//  MRController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "MRConstants.h"
#import "MPMRAIDInterstitialViewController.h"

@protocol MRControllerDelegate;
@class MPAdConfiguration;
@class CLLocation;
@class MPWebView;
@class MPViewabilityTracker;

/**
 * The `MRController` class is used to load and interact with MRAID ads.
 * It contains two MRAID ad views and uses a separate `MRBridge` to
 * communicate to each view. `MRController` handles view-related MRAID
 * native calls such as expand(), resize(), close(), and open().
 */
@interface MRController : NSObject

@property (nonatomic, weak) id<MRControllerDelegate> delegate;

- (instancetype)initWithAdViewFrame:(CGRect)adViewFrame
              supportedOrientations:(MPInterstitialOrientationType)orientationType
                    adPlacementType:(MRAdViewPlacementType)placementType
                           delegate:(id<MRControllerDelegate>)delegate;

/**
 Use this to load a regular MRAID ad.
 */
- (void)loadAdWithConfiguration:(MPAdConfiguration *)configuration;

/**
 Use this to load a VAST video companion MRAID ad.
 */
- (void)loadVASTCompanionAd:(NSString *)companionAdHTML;
- (void)loadVASTCompanionAdUrl:(NSURL *)companionAdUrl;

- (void)handleMRAIDInterstitialWillPresentWithViewController:(MPMRAIDInterstitialViewController *)viewController;
- (void)handleMRAIDInterstitialDidPresentWithViewController:(MPMRAIDInterstitialViewController *)viewController;
- (void)enableRequestHandling;
- (void)disableRequestHandling;

- (void)startViewabilityTracking;

/**
 When a click-through happens, do not open a web browser.
 Note: `MRControllerDelegate.adDidReceiveClickthrough:` is still triggered. It's the delegate's
 responsibility to open a web browser when click-through happens.
 */
- (void)disableClickthroughWebBrowser;

/**
 Evaluate the Javascript code "webviewDidAppear();" in the MRAID web view.
 */
- (void)triggerWebviewDidAppear;

@end

/**
 * The `MRControllerDelegate` will relay specific events such as ad loading to
 * the object that implements the delegate. It also requires information
 * such as adUnitId, adConfiguation, and location in order to use its
 * ad alert manager.
 **/
@protocol MRControllerDelegate <NSObject>

@required

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

// Called when click-throught happens.
- (void)adDidReceiveClickthrough:(NSURL *)url;

// Called after the rewarded video finishes playing
- (void)rewardedVideoEnded;

// Called just before the ad will expand or resize
- (void)adWillExpand:(UIView *)adView;

// Called after the ad collapsed from an expanded or resized state
- (void)adDidCollapse:(UIView *)adView;

@end
