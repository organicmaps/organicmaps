//
//  MOPUBPlayerViewController.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MOPUBPlayerView.h"

@class AVPlayerItem;
@class MOPUBAVPlayer;
@class MOPUBPlayerViewController;
@class MOPUBNativeVideoAdConfigValues;
@class MPAdConfigurationLogEventProperties;
@class MPVASTTracking;
@class MPVideoConfig;

@protocol MOPUBPlayerViewControllerDelegate <NSObject>

@optional

- (void)willEnterFullscreen:(MOPUBPlayerViewController *)viewController;
- (void)playerPlaybackWillStart:(MOPUBPlayerViewController *)player;
- (void)playerPlaybackDidStart:(MOPUBPlayerViewController *)player;
- (void)playerDidProgressToTime:(NSTimeInterval)playbackTime;
- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController didTapReplayButton:(MOPUBPlayerView *)view;
- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController willShowReplayView:(MOPUBPlayerView *)view;
- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController didStall:(MOPUBAVPlayer *)player;
- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController didRecoverFromStall:(MOPUBAVPlayer *)player;

- (UIViewController *)viewControllerForPresentingModalView;

@end

@interface MOPUBPlayerViewController : UIViewController

@property (nonatomic, readonly) NSURL *mediaURL;

@property (nonatomic, readonly) MOPUBPlayerView *playerView;
@property (nonatomic, readonly) AVPlayerItem *playerItem;
@property (nonatomic, readonly) MOPUBAVPlayer *avPlayer;
@property (nonatomic) MPVASTTracking *vastTracking;
@property (nonatomic, readonly) CGFloat videoAspectRatio;
@property (nonatomic, readonly) MOPUBNativeVideoAdConfigValues *nativeVideoAdConfig;

#pragma mark - Configurations/States
@property (nonatomic) MOPUBPlayerDisplayMode displayMode;
@property (nonatomic) BOOL muted;
@property (nonatomic) BOOL startedLoading;
@property (nonatomic) BOOL playing;
@property (nonatomic) BOOL paused;
@property (nonatomic) BOOL isReadyToPlay;
@property (nonatomic) BOOL disposed;

#pragma - Call to action click tracking url
@property (nonatomic) NSURL *defaultActionURL;

@property (nonatomic, weak) id<MOPUBPlayerViewControllerDelegate> delegate;

#pragma mark - Initializer
- (instancetype)initWithVideoConfig:(MPVideoConfig *)videoConfig nativeVideoAdConfig:(MOPUBNativeVideoAdConfigValues *)nativeVideoAdConfig logEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (void)loadAndPlayVideo;
- (void)seekToTime:(NSTimeInterval)time;
- (void)pause;
- (void)resume;
- (void)dispose;

- (BOOL)shouldStartNewPlayer;
- (BOOL)shouldResumePlayer;
- (BOOL)shouldPausePlayer;

- (void)willEnterFullscreen;
- (void)willExitFullscreen;

@end
