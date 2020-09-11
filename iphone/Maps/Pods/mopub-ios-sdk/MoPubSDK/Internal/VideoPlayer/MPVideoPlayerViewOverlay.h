//
//  MPVideoPlayerViewOverlay.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#ifndef MPVideoPlayerViewOverlay_h
#define MPVideoPlayerViewOverlay_h

#import <UIKit/UIKit.h>
#import "MPVASTCompanionAd.h"
#import "MPVideoPlayer.h"

NS_ASSUME_NONNULL_BEGIN

/**
 This is a simple data object for @c MPVideoPlayerViewOverlay.
 */
@interface MPVideoPlayerViewOverlayConfig : NSObject

/**
 Title of the Call To Action button. If nil or empty, the button is hidden.
 */
@property (nonatomic, readonly) NSString *callToActionButtonTitle;

/**
 Whether this overlay is for a rewarded video.
 */
@property (nonatomic, readonly) BOOL isRewarded;

/**
 Whether the user can perform a click-through.
 */
@property (nonatomic, readonly) BOOL isClickthroughAllowed;

/**
 Whether this video has a companion ad
 */
@property (nonatomic, readonly) BOOL hasCompanionAd;

/**
 This is for the Format Unification Phase 2 item 1.1
 clickability experiment. When the experiment is enabled, users are able to click a fullscreen
 non-rewarded VAST video ad immediately, so that they can consume additional content about the
 advertiser. Clicking on this video should launch the CTA.
 */
@property (nonatomic, readonly) BOOL enableEarlyClickthroughForNonRewardedVideo;

- (instancetype)initWithCallToActionButtonTitle:(NSString * _Nullable)callToActionButtonTitle
                                     isRewarded:(BOOL)isRewarded
                          isClickthroughAllowed:(BOOL)isClickthroughAllowed
                                 hasCompanionAd:(BOOL)hasCompanionAd
     enableEarlyClickthroughForNonRewardedVideo:(BOOL)enableEarlyClickthroughForNonRewardedVideo;

@end

/**
 @c MPVideoPlayerView has an overlay with a subset of video progress indicator, skip button, close
 button, and other UI elements. Since there are different kinds of overlay in different scenarios,
 this @c MPVideoPlayerViewOverlay protocol is here to provide a common interface for all the overlays.

 See documentation at https://developers.mopub.com/dsps/ad-formats/video/
 */
@protocol MPVideoPlayerViewOverlay <NSObject>

@optional

/**
 Initialization.
 */
- (instancetype)initWithConfig:(MPVideoPlayerViewOverlayConfig *)config;

/**
 Pause the timer.
 */
- (void)pauseTimer;

/**
 Pause the timer.
*/
- (void)resumeTimer;

/**
 @c MPVideoPlayerView calls this when the first frame of the video is played.

 Note: The provided video duration is the duration of the actual video instead of the duration
 provided in the ad response meta data (which could be inaccurate or totally wrong).
 */
- (void)handleVideoStartForSkipOffset:(MPVASTDurationOffset *)skipOffset
                        videoDuration:(NSTimeInterval)videoDuration;

/**
 Call this when the video ends.
 */
- (void)handleVideoComplete;

/**
 Show the industry icon.
 */
- (void)showIndustryIcon:(MPVASTIndustryIcon *)icon;

/**
 Hide the industry icon.
 */
- (void)hideIndustryIcon;

@end

@protocol MPVideoPlayerViewOverlayDelegate <NSObject>

- (void)videoPlayerViewOverlay:(id<MPVideoPlayerViewOverlay>)overlay didTriggerEvent:(MPVideoPlayerEvent)event;

@end

NS_ASSUME_NONNULL_END

#endif /* MPVideoPlayerViewOverlay_h */
