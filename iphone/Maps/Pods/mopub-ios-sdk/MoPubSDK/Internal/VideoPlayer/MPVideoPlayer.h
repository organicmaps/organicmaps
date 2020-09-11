//
//  MPVideoPlayer.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#ifndef MPVideoPlayer_h
#define MPVideoPlayer_h

#import "MPVideoConfig.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MPVideoPlayerEvent) {
    MPVideoPlayerEvent_ClickThrough,
    MPVideoPlayerEvent_Close,
    MPVideoPlayerEvent_Skip
};

@protocol MPVideoPlayer <NSObject>

/**
 A @c MPVideoPlayerViewDelegate is required for this view to work properly. This init loads the
 video right away and the delegate is notified as soon as the video is loaded and ready to play.
 */
- (instancetype)initWithVideoURL:(NSURL *)videoURL videoConfig:(MPVideoConfig *)videoConfig;

/**
 Load the video.
 
 Note: Call this for once at most. Subsequent calls after the first one will have no effect.
 */
- (void)loadVideo;

/**
 Play the video.
 */
- (void)play;

/**
 Pause the video.
 */
- (void)pause;

/**
 Enable app life cycle event observation for auto-play and auto-pause.
 Note: This should be called when the video is visible (for example, not blocked by click-through web view).
 */
- (void)enableAppLifeCycleEventObservationForAutoPlayPause;

/**
 Disable app life cycle event observation for auto-play and auto-pause.
 Note: This should be called when the video is invisible (for example, blocked by click-through web view).
 */
- (void)disableAppLifeCycleEventObservationForAutoPlayPause;

@end

NS_ASSUME_NONNULL_END

#endif /* MPVideoPlayer_h */
