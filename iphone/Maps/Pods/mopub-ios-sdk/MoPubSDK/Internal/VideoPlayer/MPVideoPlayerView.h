//
//  MPVideoPlayerView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPVideoPlayer.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPVideoPlayerViewDelegate;

/**
 @c MPVideoPlayerView only allows start playing without pause, reset, nor fast forwarding. Video is
 only paused automatically due to app life cycle events or user interactions such as click-throughs.
 
 Note: The actually video duration is honored as the source of truth, while the video duration
 provided by the @c MPVideoConfig is ignored.
 */
@interface MPVideoPlayerView : UIView <MPVideoPlayer>

@property (nonatomic, weak) id<MPVideoPlayerViewDelegate> delegate;
@property (nonatomic, readonly) BOOL didLoadVideo; // set to YES after `loadVideo` is called for the first time; never set back to NO again
@property (nonatomic, readonly) BOOL hasStartedPlaying; // set to YES after `play` is called for the first time; never set back to NO again
@property (nonatomic, readonly) NSTimeInterval videoDuration;
@property (nonatomic, readonly) NSTimeInterval videoProgress;

@end

#pragma mark -

@protocol MPVideoPlayerViewDelegate <NSObject>

- (void)videoPlayerViewDidLoadVideo:(MPVideoPlayerView *)videoPlayerView;

- (void)videoPlayerViewDidFailToLoadVideo:(MPVideoPlayerView *)videoPlayerView error:(NSError *)error;

- (void)videoPlayerViewDidStartVideo:(MPVideoPlayerView *)videoPlayerView duration:(NSTimeInterval)duration;

- (void)videoPlayerViewDidCompleteVideo:(MPVideoPlayerView *)videoPlayerView duration:(NSTimeInterval)duration;

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
videoDidReachProgressTime:(NSTimeInterval)videoProgress
               duration:(NSTimeInterval)duration;

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
        didTriggerEvent:(MPVideoPlayerEvent)event
          videoProgress:(NSTimeInterval)videoProgress;

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
       showIndustryIcon:(MPVASTIndustryIcon *)icon;

- (void)videoPlayerViewHideIndustryIcon:(MPVideoPlayerView *)videoPlayerView;

@end

NS_ASSUME_NONNULL_END
