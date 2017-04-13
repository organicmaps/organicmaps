//
//  MOPUBPlayerViewController.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import "MOPUBAVPlayer.h"
#import "MOPUBFullscreenPlayerViewController.h"
#import "MOPUBPlayerViewController.h"
#import "MOPUBActivityIndicatorView.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPLogEvent+NativeVideo.h"
#import "MPLogEventRecorder.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MPVastTracking.h"
#import "MPVideoConfig.h"
#import "UIView+MPAdditions.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MPVideoConfig.h"
#import "UIButton+MPAdditions.h"

#define kDefaultVideoAspectRatio 16.0f/9.0f

static NSString * const kMutedButtonImage = @"MPMutedBtn.png";
static NSString * const kUnmutedButtonImage = @"MPUnmutedBtn.png";

static NSString * const kTracksKey = @"tracks";
static NSString * const kPlayableKey = @"playable";

// playerItem keys
static NSString * const kStatusKey = @"status";
static NSString * const kCurrentItemKey = @"currentItem";
static NSString * const kLoadedTimeRangesKey = @"loadedTimeRanges";
static void *AudioControllerBufferingObservationContext = &AudioControllerBufferingObservationContext;

// UI specifications
static CGFloat const kMuteIconInlineModeLeftMargin = 6.0f;
static CGFloat const kMuteIconInlineModeBottomMargin = 5.0f;
static CGFloat const kMuteIconInlineModeTouchAreaInsets = 25.0f;

static CGFloat const kLoadingIndicatorTopMargin = 8.0f;
static CGFloat const kLoadingIndicatorRightMargin = 8.0f;

// force resume playback in 3 seconds. player might get stuck due to stalled item
static CGFloat const kDelayPlayInSeconds = 3.0f;

// We compare the buffered time to the length of the video to determine when it has been
// fully buffered. To account for rounding errors, allow a small error when making this
// calculation.
static const double kVideoFinishedBufferingAllowedError = 0.1;

@interface MOPUBPlayerViewController() <MOPUBAVPlayerDelegate, MOPUBPlayerViewDelegate, MPAdDestinationDisplayAgentDelegate>

@property (nonatomic) UIButton *muteButton;
@property (nonatomic) UIActivityIndicatorView *loadingIndicator;
@property (nonatomic) MPAdDestinationDisplayAgent *displayAgent;

// KVO might be triggerd multipe times. This property is used to make sure the view will only be created once.
@property (nonatomic) BOOL alreadyInitialized;
@property (nonatomic) MPAdConfigurationLogEventProperties *logEventProperties;
@property (nonatomic) BOOL downloadFinishedEventFired;
@property (nonatomic) BOOL alreadyCreatedPlayerView;
@property (nonatomic) BOOL finishedPlaying;

@end

@implementation MOPUBPlayerViewController

- (instancetype)initWithVideoConfig:(MPVideoConfig *)videoConfig nativeVideoAdConfig:(MOPUBNativeVideoAdConfigValues *)nativeVideoAdConfig logEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties
{
    if (self = [super init]) {
        _mediaURL = videoConfig.mediaURL;
        _playerView = [[MOPUBPlayerView alloc] initWithFrame:CGRectZero delegate:self];
        self.displayMode = MOPUBPlayerDisplayModeInline;

        NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
        [notificationCenter addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
        [notificationCenter addObserver:self selector:@selector(applicationDidEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
        _nativeVideoAdConfig = nativeVideoAdConfig;
        _logEventProperties = logEventProperties;

        // default aspect ratio is 16:9
        _videoAspectRatio = kDefaultVideoAspectRatio;
    }
    return self;
}

#pragma mark - UIViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self.view addSubview:self.playerView];
    [self startLoadingIndicator];
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];

    // Bring mute button to front. This is necessary because the video view might be detached
    // and re-attached during fullscreen to in-feed transition
    [self.view bringSubviewToFront:self.muteButton];
    // Set playerView's frame so it will work for rotation
    self.playerView.frame = self.view.bounds;

    [self layoutLoadingIndicator];
}

- (void)layoutLoadingIndicator
{
    if (_loadingIndicator) {
        _loadingIndicator.mp_x = CGRectGetWidth(self.view.bounds) - kLoadingIndicatorRightMargin - CGRectGetWidth(_loadingIndicator.bounds);
        _loadingIndicator.mp_y = kLoadingIndicatorTopMargin;
    }
}

#pragma mark - dealloc or dispose the controller

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    if (self.avPlayer) {
        [self.avPlayer removeObserver:self forKeyPath:kStatusKey];
    }

    if (self.playerItem) {
        [self.playerItem removeObserver:self forKeyPath:kStatusKey];
        [self.playerItem removeObserver:self forKeyPath:kLoadedTimeRangesKey];
    }

    MPLogDebug(@"playerViewController dealloc called");
}

- (void)dispose
{
    [self.view removeFromSuperview];
    [self.avPlayer dispose];
    self.avPlayer = nil;
    self.disposed = YES;
}

#pragma mark - load asset, set up aVplayer and avPlayer view

- (void)handleVideoInitError
{
    MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeErrorFailedToPlay]);
    [self.vastTracking handleVideoEvent:MPVideoEventTypeError videoTimeOffset:self.avPlayer.currentPlaybackTime];
    [self stopLoadingIndicator];
    [self.playerView handleVideoInitFailure];
}

- (void)loadAndPlayVideo
{
    self.startedLoading = YES;

    AVURLAsset *asset = [[AVURLAsset alloc] initWithURL:self.mediaURL options:nil];

    if (asset == nil) {
        MPLogError(@"failed to initialize video asset for URL %@", self.mediaURL);
        [self handleVideoInitError];

        return;
    }

    MPAddLogEvent([[MPLogEvent alloc ] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeDownloadStart]);

    NSArray *requestedKeys = @[kTracksKey, kPlayableKey];
    [asset loadValuesAsynchronouslyForKeys:requestedKeys completionHandler:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!self.disposed) {
                [self prepareToPlayAsset:asset withKeys:requestedKeys];
            }
        });
    }];
}

- (void)setVideoAspectRatioWithAsset:(AVURLAsset *)asset
{
    if (asset && [asset tracksWithMediaType:AVMediaTypeVideo].count > 0) {
        AVAssetTrack *videoTrack = [asset tracksWithMediaType:AVMediaTypeVideo][0];
        CGSize naturalSize = CGSizeApplyAffineTransform(videoTrack.naturalSize, videoTrack.preferredTransform);
        naturalSize = CGSizeMake(fabs(naturalSize.width), fabs(naturalSize.height));

        // make sure the natural size is at least 1pt (not 0) check
        if (naturalSize.height > 0 && naturalSize.width > 0) {
            _videoAspectRatio = naturalSize.width / naturalSize.height;
        }
    }
}

- (void)prepareToPlayAsset:(AVURLAsset *)asset withKeys:(NSArray *)requestedKeys
{
    NSError *error = nil;

    if (!asset.playable) {
        MPLogError(@"asset is not playable");
        [self handleVideoInitError];

        return;
    }

    AVKeyValueStatus status = [asset statusOfValueForKey:kTracksKey error:&error];
    if (status == AVKeyValueStatusFailed) {
        MPLogError(@"AVKeyValueStatusFailed");
        [self handleVideoInitError];

        return;
    } else if (status == AVKeyValueStatusLoaded) {
        [self setVideoAspectRatioWithAsset:asset];

        self.playerItem = [AVPlayerItem playerItemWithAsset:asset];
        self.avPlayer = [[MOPUBAVPlayer alloc] initWithDelegate:self playerItem:self.playerItem];
        self.avPlayer.muted = YES;

        [self.playerView setAvPlayer:self.avPlayer];
    }
}

#pragma mark - video ready to play
- (void)initOnVideoReady
{
    [self startPlayer];
    MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeVideoReady]);
}

- (void)createView
{
    [self.playerView createPlayerView];
    [self createMuteButton];
}

- (void)createMuteButton
{
    if (!self.muteButton) {
        self.muteButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [self.muteButton setImage:[UIImage imageNamed:MPResourcePathForResource(kMutedButtonImage)] forState:UIControlStateNormal];
        [self.muteButton setImage:[UIImage imageNamed:MPResourcePathForResource(kUnmutedButtonImage)] forState:UIControlStateSelected];
        [self.muteButton addTarget:self action:@selector(muteButtonTapped) forControlEvents:UIControlEventTouchUpInside];
        self.muteButton.mp_TouchAreaInsets = UIEdgeInsetsMake(kMuteIconInlineModeTouchAreaInsets, kMuteIconInlineModeTouchAreaInsets, kMuteIconInlineModeTouchAreaInsets, kMuteIconInlineModeTouchAreaInsets);
        [self.muteButton sizeToFit];
        [self.view addSubview:self.muteButton];
        self.muteButton.frame = CGRectMake(kMuteIconInlineModeLeftMargin, CGRectGetMaxY(self.view.bounds) -  kMuteIconInlineModeBottomMargin - CGRectGetHeight(self.muteButton.bounds), CGRectGetWidth(self.muteButton.bounds), CGRectGetHeight(self.muteButton.bounds));
    }
}

- (void)startPlayer
{
    [self.avPlayer play];
    self.playing = YES;
    self.isReadyToPlay = YES;
    if ([self.delegate respondsToSelector:@selector(playerPlaybackDidStart:)]) {
        [self.delegate playerPlaybackDidStart:self];
    }
}

#pragma mark - displayAgent

- (MPAdDestinationDisplayAgent *)displayAgent
{
    if (!_displayAgent) {
        _displayAgent = [[MPCoreInstanceProvider sharedProvider] buildMPAdDestinationDisplayAgentWithDelegate:self];
    }
    return _displayAgent;
}


#pragma mark - setter for player related objects

- (void)setPlayerItem:(AVPlayerItem *)playerItem
{
    if (_playerItem) {
        [_playerItem removeObserver:self forKeyPath:kStatusKey];
        [_playerItem removeObserver:self forKeyPath:kLoadedTimeRangesKey];
    }
    _playerItem = playerItem;
    if (!playerItem) {
        return;
    }

    [_playerItem addObserver:self forKeyPath:kStatusKey options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew context:nil];
    [_playerItem addObserver:self
                  forKeyPath:kLoadedTimeRangesKey
                     options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                     context:AudioControllerBufferingObservationContext];
}

- (void)setAvPlayer:(MOPUBAVPlayer *)avPlayer
{
    if (_avPlayer) {
        [_avPlayer removeObserver:self forKeyPath:kStatusKey];
    }
    _avPlayer = avPlayer;
    if (_avPlayer) {
        NSKeyValueObservingOptions options = (NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew);
        [_avPlayer addObserver:self forKeyPath:kStatusKey options:options context:nil];
    }
}

- (void)setMuted:(BOOL)muted
{
    _muted = muted;
    [self.muteButton setSelected:!muted];
    self.avPlayer.muted = muted;
}

#pragma mark - displayMode

- (MOPUBPlayerDisplayMode)displayMode
{
    return self.playerView.displayMode;
}

- (void)setDisplayMode:(MOPUBPlayerDisplayMode)displayMode
{
    self.playerView.displayMode = displayMode;
    if (displayMode == MOPUBPlayerDisplayModeInline) {
        self.muted = YES;
    } else {
        self.muted = NO;
    }
}

#pragma mark - acvivityIndicator
- (UIActivityIndicatorView *)loadingIndicator
{
    if (!_loadingIndicator) {
        _loadingIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        _loadingIndicator.hidesWhenStopped = YES;
        _loadingIndicator.color = [UIColor whiteColor];
        [self.view addSubview:_loadingIndicator];
    }
    return _loadingIndicator;
}

- (void)startLoadingIndicator
{
    [self.loadingIndicator.superview bringSubviewToFront:_loadingIndicator];
    [self.loadingIndicator startAnimating];
}

- (void)stopLoadingIndicator
{
    if (_loadingIndicator && _loadingIndicator.isAnimating) {
        [_loadingIndicator stopAnimating];
    }
}

- (void)removeLoadingIndicator
{
    if (_loadingIndicator) {
        [_loadingIndicator stopAnimating];
        [_loadingIndicator removeFromSuperview];
        _loadingIndicator = nil;
    }
}


#pragma mark - Tap actions

- (void)muteButtonTapped
{
    self.muteButton.selected = !self.muteButton.selected;
    self.muted = !self.muteButton.selected;

    MPVideoEventType eventType = self.muted ? MPVideoEventTypeMuted : MPVideoEventTypeUnmuted;
    [self.vastTracking handleVideoEvent:eventType videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

# pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (object == self.avPlayer) {
        if (self.avPlayer.status == AVPlayerItemStatusFailed) {
            if (self.isReadyToPlay) {
                MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeErrorDuringPlayback]);
            } else {
                MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeErrorFailedToPlay]);
            }
            MPLogError(@"avPlayer status failed");
            [self.vastTracking handleVideoEvent:MPVideoEventTypeError videoTimeOffset:self.avPlayer.currentPlaybackTime];
        }
    } else if (object == self.playerItem) {
        if (context == AudioControllerBufferingObservationContext) {
            NSArray *timeRangeArray = [self.playerItem loadedTimeRanges];
            if (timeRangeArray && timeRangeArray.count > 0) {
                CMTimeRange aTimeRange = [[timeRangeArray objectAtIndex:0] CMTimeRangeValue];
                double startTime = CMTimeGetSeconds(aTimeRange.start);
                double loadedDuration = CMTimeGetSeconds(aTimeRange.duration);
                double videoDuration = CMTimeGetSeconds(self.playerItem.duration);
                if ((startTime + loadedDuration + kVideoFinishedBufferingAllowedError) >= videoDuration && !self.downloadFinishedEventFired) {
                    self.downloadFinishedEventFired = YES;
                    MPAddLogEvent([[MPLogEvent alloc ] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeDownloadFinished]);
                }
            }
        }
        if ([keyPath isEqualToString:kStatusKey]) {
            switch (self.playerItem.status) {
                case AVPlayerItemStatusReadyToPlay:
                    self.vastTracking.videoDuration = CMTimeGetSeconds(self.playerItem.duration);
                    if (!self.alreadyInitialized) {
                        self.alreadyInitialized = YES;
                        [self initOnVideoReady];
                    }
                    break;
                case AVPlayerItemStatusFailed:
                {
                    MPLogError(@"avPlayerItem status failed");
                    [self.vastTracking handleVideoEvent:MPVideoEventTypeError videoTimeOffset:self.avPlayer.currentPlaybackTime];
                    break;
                }
                default:
                    break;
            }
        }
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - player controls

- (void)pause
{
    self.paused = YES;
    self.playing = NO;
    [self.avPlayer pause];
    [self.vastTracking handleVideoEvent:MPVideoEventTypePause videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

- (void)resume
{
    self.paused = NO;
    self.playing = YES;
    [self.avPlayer play];
    [self.vastTracking handleVideoEvent:MPVideoEventTypeResume videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

- (void)seekToTime:(NSTimeInterval)time
{
    [self.avPlayer seekToTime:CMTimeMakeWithSeconds(time, NSEC_PER_SEC) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

#pragma mark - auto play helper method
- (BOOL)shouldStartNewPlayer
{
    UIApplicationState state = [[UIApplication sharedApplication] applicationState];
    if (!self.startedLoading && !self.playing && !self.paused && state == UIApplicationStateActive) {
        return YES;
    }
    return NO;
}

- (BOOL)shouldResumePlayer
{
    UIApplicationState state = [[UIApplication sharedApplication] applicationState];
    if (self.startedLoading && self.paused == YES && self.displayMode == MOPUBPlayerDisplayModeInline
        && state == UIApplicationStateActive) {
        return YES;
    }
    return NO;
}

- (BOOL)shouldPausePlayer
{
    if (self.playing && self.displayMode == MOPUBPlayerDisplayModeInline) {
        return YES;
    }
    return NO;
}

#pragma mark - enter fullscreen or exit fullscreen

- (void)willEnterFullscreen
{
    self.displayMode = MOPUBPlayerDisplayModeFullscreen;
    [self.vastTracking handleVideoEvent:MPVideoEventTypeFullScreen videoTimeOffset:self.avPlayer.currentPlaybackTime];
    [self.vastTracking handleVideoEvent:MPVideoEventTypeExpand videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

- (void)willExitFullscreen
{
    self.displayMode = MOPUBPlayerDisplayModeInline;
    [self.vastTracking handleVideoEvent:MPVideoEventTypeExitFullScreen videoTimeOffset:self.avPlayer.currentPlaybackTime];
    [self.vastTracking handleVideoEvent:MPVideoEventTypeCollapse videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

#pragma mark - MOPUBAVPlayerDelegate

- (void)avPlayer:(MOPUBAVPlayer *)player playbackTimeDidProgress:(NSTimeInterval)currentPlaybackTime
{
    // stop the loading indicator if it exists and is animating.
    [self stopLoadingIndicator];

    // When the KVO sends AVPlayerItemStatusReadyToPlay, there could still be a delay for the video really starts playing.
    // If we create the mute button and progress bar immediately after AVPlayerItemStatusReadyToPlay signal, we might
    // end up with showing them before the video is visible. To prevent that, we create mute button and progress bar here.
    // There will be 0.1s delay after the video starts playing, but it's a much better user experience.

    if (!self.alreadyCreatedPlayerView) {
        [self createView];
        self.alreadyCreatedPlayerView = YES;
    }

    [self.playerView playbackTimeDidProgress];

    if ([self.delegate respondsToSelector:@selector(playerDidProgressToTime:)]) {
        [self.delegate playerDidProgressToTime:currentPlaybackTime];
    }
}

- (void)avPlayer:(MOPUBAVPlayer *)player didError:(NSError *)error withMessage:(NSString *)message
{
    [self.avPlayer pause];
    MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeErrorDuringPlayback]);
    [self.vastTracking handleVideoEvent:MPVideoEventTypeError videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

- (void)avPlayerDidFinishPlayback:(MOPUBAVPlayer *)player
{
    self.finishedPlaying = YES;
    [self removeLoadingIndicator];
    [self.avPlayer pause];
    // update view
    [self.playerView playbackDidFinish];
    [self.vastTracking handleVideoEvent:MPVideoEventTypeCompleted videoTimeOffset:self.avPlayer.currentPlaybackTime];
}

- (void)avPlayerDidRecoverFromStall:(MOPUBAVPlayer *)player
{
    if (self.displayMode == MOPUBPlayerDisplayModeInline) {
        [self removeLoadingIndicator];
    } else {
        if ([self.delegate respondsToSelector:@selector(playerViewController:didRecoverFromStall:)]) {
            [self.delegate playerViewController:self didRecoverFromStall:player];
        }
    }

    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(resume) object:nil];
}

- (void)avPlayerDidStall:(MOPUBAVPlayer *)player
{
    MPAddLogEvent([[MPLogEvent alloc] initWithLogEventProperties:self.logEventProperties nativeVideoEventType:MPNativeVideoEventTypeBuffering]);
    if (self.displayMode == MOPUBPlayerDisplayModeInline) {
        [self startLoadingIndicator];
    } else {
        if ([self.delegate respondsToSelector:@selector(playerViewController:didStall:)]) {
            [self.delegate playerViewController:self didStall:self.avPlayer];
        }
    }

    // Try to resume the video play after 3 seconds. The perform selector request is cancelled when
    // didRecoverFromStall signal is received. This way, we won't queue up the requests.
    [self performSelector:@selector(resume) withObject:nil afterDelay:kDelayPlayInSeconds];
}

#pragma mark - MOPUBPlayerViewDelegate
- (void)playerViewDidTapReplayButton:(MOPUBPlayerView *)view
{
    self.muteButton.hidden = NO;
    self.finishedPlaying = NO;
    [self.playerView setProgressBarVisible:YES];
    [self seekToTime:0];
    [self.avPlayer play];
}

- (void)playerViewWillShowReplayView:(MOPUBPlayerView *)view
{
    self.muteButton.hidden = YES;
    [self.playerView setProgressBarVisible:NO];
    if (self.displayMode == MOPUBPlayerDisplayModeFullscreen) {
        if ([self.delegate respondsToSelector:@selector(playerViewController:didTapReplayButton:)]) {
            [self.delegate playerViewController:self willShowReplayView:self.playerView];
        }
    }
}

- (void)playerViewWillEnterFullscreen:(MOPUBPlayerView *)view
{
    if ([self.delegate respondsToSelector:@selector(willEnterFullscreen:)]) {
        [self.delegate willEnterFullscreen:self];
    }
}

#pragma mark - Application state monitoring

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
    if (self.avPlayer && self.avPlayer.rate > 0) {
        [self pause];
    }
}

- (void)applicationDidEnterForeground:(NSNotification *)notification
{
    // Resume video playback only if the visible area is larger than or equal to the autoplay threshold.

    BOOL playVisible = MPViewIntersectsParentWindowWithPercent(self.playerView, self.nativeVideoAdConfig.playVisiblePercent/100.0f);
    if (self.avPlayer && self.isReadyToPlay && !self.finishedPlaying && playVisible) {
        [self resume];
    }
}

#pragma mark - <MPAdDestinationDisplayAgent>

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)displayAgentWillPresentModal
{
    [self pause];
}

- (void)displayAgentWillLeaveApplication
{
    [self pause];
}

- (void)displayAgentDidDismissModal
{
    [self resume];
}

@end

