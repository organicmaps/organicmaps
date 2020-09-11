//
//  MPVideoPlayerView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <AVFoundation/AVFoundation.h>
#import "MPError.h"
#import "MPLogging.h"
#import "MPVASTTracking.h"
#import "MPVideoPlayerView.h"
#import "UIColor+MPAdditions.h"
#import "UIView+MPAdditions.h"

static void * KVOContext = &KVOContext;

static NSString * const kProgressBarFillColor = @"#FFCC4D";

// 600 is recommended by Apple since it is a multiple of common frame rates such as 24 FPS, 25 FPS,
// and 60 FPS. See "AVFoundation Programming Guide" -> "Time and Media Representations".
int32_t const kPreferredTimescale = 600;

// play time is approximate thus we need to consider tolerance
NSTimeInterval const kPlayTimeTolerance = 0.1;

@interface MPVideoPlayerView ()

@property (nonatomic, strong) NSURL *videoURL;
@property (nonatomic, strong) MPVideoConfig *videoConfig;
@property (nonatomic, strong) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) BOOL didLoadVideo; // set to YES after `loadVideo` is called for the first time; never set back to NO again
@property (nonatomic, assign) BOOL hasStartedPlaying; // set to YES after `play` is called for the first time; never set back to NO again
@property (nonatomic, assign) BOOL didPlayToEndTime; // set to YES after the video ended
@property (nonatomic, assign) BOOL isAutoPlayPauseEnabled;
@property (nonatomic, assign) BOOL didFireStartEvent;
@property (nonatomic, strong) UIProgressView *progressBar;
@property (nonatomic, strong) NSLayoutConstraint *progressBarTopConstraint;

@property (nonatomic, strong) id<NSObject> progressBarTimeObserver;
@property (nonatomic, strong) id<NSObject> progressTrackingTimeObserver;
@property (nonatomic, strong) id<NSObject> boundaryTrackingTimeObserver;
@property (nonatomic, strong) id<NSObject> industryIconShowTimeObserver;
@property (nonatomic, strong) id<NSObject> industryIconHideTimeObserver;
@property (nonatomic, strong) id<NSObject> endTimeObserverToken;
@property (nonatomic, strong) id<NSObject> audioSessionInterruptionObserverToken;

@end

@implementation MPVideoPlayerView

- (void)dealloc {
    [self.player pause]; // just in case the player session does not stop in time
    [self.player removeTimeObserver: self.progressBarTimeObserver];
    [self.player removeTimeObserver: self.progressTrackingTimeObserver];
    [self.player removeTimeObserver: self.boundaryTrackingTimeObserver];
    [self.player removeTimeObserver: self.industryIconShowTimeObserver];
    [self.player removeTimeObserver: self.industryIconHideTimeObserver];
    [self.notificationCenter removeObserver:self];
    [self.notificationCenter removeObserver:self.endTimeObserverToken];
    [self.notificationCenter removeObserver:self.audioSessionInterruptionObserverToken];

    // Note: disable KVO for the layer before `[self.player replaceCurrentItemWithPlayerItem:nil];`
    // otherwise app will crash because the progress bar would have been deallocated.
    [self.playerLayer removeObserver:self forKeyPath:NSStringFromSelector(@selector(videoRect))];

    // Audio session might be messed up without setting current item to nil.
    // Note: remove current item from KVO first to avoid crash!
    [self.player.currentItem removeObserver:self forKeyPath:NSStringFromSelector(@selector(duration))];
    [self.player replaceCurrentItemWithPlayerItem:nil];
}

- (NSTimeInterval)videoDuration {
    return CMTimeGetSeconds(self.player.currentItem.duration);
}

- (NSTimeInterval)videoProgress {
    return CMTimeGetSeconds(self.player.currentTime);
}

#pragma mark - MPVideoPlayer

- (instancetype)initWithVideoURL:(NSURL *)videoURL videoConfig:(MPVideoConfig *)videoConfig {
    if (self = [super init]) {
        _videoURL = videoURL;
        _videoConfig = videoConfig;
        _notificationCenter = [NSNotificationCenter defaultCenter];
        self.backgroundColor = UIColor.blackColor;
    }
    return self;
}

- (void)loadVideo {
    if (self.didLoadVideo) {
        return;
    }
    self.didLoadVideo = YES;

    [self setUpVideoPlayer];
    [self setUpProgressBar];
}

- (void)play {
    if (self.hasStartedPlaying == NO) {
        self.hasStartedPlaying = YES;

        [self observeProgressTimeForTracking];
        [self observeBoundaryTimeForTracking];
        [self observeBoundaryTimeForIndustryIcons:self.videoConfig.industryIcons
                                    videoDuration:self.videoDuration];
        [self enableAppLifeCycleEventObservationForAutoPlayPause];
    }

    [self.player play];
}

- (void)pause {
    [self.player pause];
}

- (void)enableAppLifeCycleEventObservationForAutoPlayPause {
    [self.notificationCenter addObserver:self
                                selector:@selector(pause)
                                    name:UIApplicationDidEnterBackgroundNotification
                                  object:nil];
    [self.notificationCenter addObserver:self
                                selector:@selector(play)
                                    name:UIApplicationWillEnterForegroundNotification
                                  object:nil];
    self.isAutoPlayPauseEnabled = YES;
}

- (void)disableAppLifeCycleEventObservationForAutoPlayPause {
    [self.notificationCenter removeObserver:self
                                       name:UIApplicationDidEnterBackgroundNotification
                                     object:nil];
    [self.notificationCenter removeObserver:self
                                       name:UIApplicationWillEnterForegroundNotification
                                     object:nil];
    self.isAutoPlayPauseEnabled = NO;
}

#pragma mark - Private Methods

/**
 Use `AVPlayerLayer` instead of `CALayer` for the backing layer.
 */
+ (Class)layerClass {
    return [AVPlayerLayer class];
}

/**
 A helper for saving type casting effort.
 */
- (AVPlayerLayer *)playerLayer {
    return (AVPlayerLayer *)self.layer;
}

/**
 A helper for easier player access.
 */
- (AVPlayer *)player {
    return self.playerLayer.player;
}

/**
 A helper for setting up the @c player of @c playerLayer. Call this during init only.
 */
- (void)setUpVideoPlayer {
    if (self.playerLayer.player != nil) {
        MPLogDebug(@"video player has been set up");
        return;
    }

    AVPlayer *player = [AVPlayer playerWithURL:self.videoURL];
    // `AVPlayerStatusReadyToPlay` alone is not reliable for observing video duration (could
    // still be NaN when ready to play). Observe the duration of the player item instead.
    [player.currentItem addObserver:self
                         forKeyPath:NSStringFromSelector(@selector(duration))
                            options:0
                            context:KVOContext];
    self.playerLayer.player = player;
    self.playerLayer.videoGravity = AVLayerVideoGravityResizeAspect;

    [self observeProgressTimeForProgressBar];
}

- (void)setUpProgressBar {
    if (self.progressBar != nil) {
        MPLogDebug(@"video player progress bar has been set up");
        return;
    }

    // KVO for porgress bar layout
    [self.playerLayer addObserver:self
                       forKeyPath:NSStringFromSelector(@selector(videoRect))
                          options:0
                          context:KVOContext];

    UIProgressView *progressBar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    self.progressBar = progressBar;
    self.progressBar.progressTintColor = [UIColor mp_colorFromHexString:kProgressBarFillColor alpha:1];

    // Progress bar ignores the safe area of the player view because the video player uses the whole
    // area that could be more than the safe area (very likely in landscape mode), and we don't want
    // the progress bar being somewhere in the middle of the video.
    [self addSubview:progressBar];
    progressBar.translatesAutoresizingMaskIntoConstraints = NO;
    self.progressBarTopConstraint = [progressBar.topAnchor constraintEqualToAnchor:self.topAnchor];
    [self.progressBarTopConstraint setActive:YES];
    [[progressBar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor] setActive:YES];
    [[progressBar.trailingAnchor constraintEqualToAnchor:self.trailingAnchor] setActive:YES];
    progressBar.hidden = YES; // hide it before we know how to position it (by obtaining valid video rect)
}

- (void)updateProgressBarProgress {
    NSTimeInterval progress = CMTimeGetSeconds(self.player.currentItem.currentTime);
    NSTimeInterval duration = CMTimeGetSeconds(self.player.currentItem.duration);

    if (duration <= 0) {
        return;
    }

    [self.progressBar setProgress:(progress / duration) animated:YES];
}

#pragma mark - Observation

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context {
    if (context != KVOContext) {
        MPLogError(@"KVO context not expected");
        return;
    }

    if (object == self.player.currentItem
        && [keyPath isEqualToString:NSStringFromSelector(@selector(duration))]) {
        // `AVPlayerStatusReadyToPlay` alone is not reliable for observing video duration (could
        // still be NaN when ready to play). Observe the duration of the player item instead.
        switch (self.player.status) {
            case AVPlayerStatusUnknown: {
                break; // no op
            }
            case AVPlayerStatusReadyToPlay: {
                if (!CMTIME_IS_INDEFINITE(self.player.currentItem.duration)) {
                    MPLogInfo(@"Ready to play video [%.3fs]", CMTimeGetSeconds(self.player.currentItem.duration));
                    [self.delegate videoPlayerViewDidLoadVideo:self];
                }
                break;
            }
            case AVPlayerStatusFailed: {
                NSError *error = [NSError errorWithCode:MOPUBErrorVideoPlayerFailedToPlay
                                   localizedDescription:@"Error: AVPlayerStatusFailed"];
                [self.delegate videoPlayerViewDidFailToLoadVideo:self error:error];
                break;
            }
        }
    } else if (object == self.playerLayer
               && [keyPath isEqualToString:NSStringFromSelector(@selector(videoRect))]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            self.progressBar.hidden = NO;

            // put progress to the bottom of the video
            CGFloat y = CGRectGetMaxY(self.playerLayer.videoRect) - self.progressBar.frame.size.height;
            self.progressBarTopConstraint.constant = y;
            [self.progressBar setNeedsUpdateConstraints];
        });
    } else {
        MPLogError(@"Key path [%@] is observed but not handled", keyPath);
    }
}

- (void)observeProgressTimeForProgressBar {
    if (self.progressBarTimeObserver) {
        MPLogDebug(@"Player progress time has been observed for progress bar");
        return;
    }

    __weak __typeof__(self) weakSelf = self;
    [self.player addPeriodicTimeObserverForInterval:CMTimeMakeWithSeconds(0.1, kPreferredTimescale)
                                              queue:dispatch_get_main_queue()
                                         usingBlock:^(CMTime time) {
                                             [weakSelf updateProgressBarProgress];
                                         }];
}

/**
 Observe progress time defined in the progress trackers.
 */
- (void)observeProgressTimeForTracking {
    if (self.progressTrackingTimeObserver) {
        MPLogDebug(@"Player progress time has been observed for tracking");
        return;
    }

    NSTimeInterval duration = CMTimeGetSeconds(self.player.currentItem.duration);
    NSMutableArray<NSValue *> *checkpoints = [NSMutableArray new];

    for (MPVASTTrackingEvent *event in [self.videoConfig trackingEventsForKey:MPVideoEventProgress]) {
        NSTimeInterval time = [event.progressOffset timeIntervalForVideoWithDuration:duration];
        [checkpoints addObject:[NSValue valueWithCMTime:CMTimeMakeWithSeconds(time, kPreferredTimescale)]];
    }

    // if `checkpoints` is empty, `addBoundaryTimeObserverForTimes:queue:usingBlock:` will crash
    if (checkpoints.count == 0) {
        return;
    }

    __weak __typeof__(self) weakSelf = self;
    void (^observationHandler)(void) = ^void() {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }

        [strongSelf.delegate videoPlayerView:strongSelf
                   videoDidReachProgressTime:CMTimeGetSeconds(strongSelf.player.currentTime)
                                    duration:CMTimeGetSeconds(strongSelf.player.currentItem.duration)];
    };

    // `addBoundaryTimeObserverForTimes` has undefined behavior with concurrent queue obtained
    // from `dispatch_get_global_queue`, thus use the main queue here since it's serial.
    self.progressTrackingTimeObserver = [self.player addBoundaryTimeObserverForTimes:checkpoints
                                                                               queue:dispatch_get_main_queue()
                                                                          usingBlock:observationHandler];
}

/**
 Observe 0/4, 1/4, 2/4. 3/4, and 4/4 boundary time of the video playing progress.
 */
- (void)observeBoundaryTimeForTracking {
    if (self.boundaryTrackingTimeObserver) {
        MPLogDebug(@"Player boundary time has been observed");
        return;
    }

    // `addBoundaryTimeObserverForTimes` might skip event when observing the ending moment (playing
    // the last frame). This is probably similar to how `NSTimer` works with run loop. As a result,
    // `AVPlayerItemDidPlayToEndTimeNotification` is observed instead, and `duration` is not added
    // to the `checkpoint` array for the END event.
    CMTime duration = self.player.currentItem.duration; // do not use this as a checkpoint
    NSTimeInterval durationInSeconds = CMTimeGetSeconds(duration);
    CMTime startTime = CMTimeMakeWithSeconds(0.01, kPreferredTimescale); // setting to 0 second won't work
    CMTime firstQuarterTime = CMTimeMultiplyByFloat64(duration, 0.25);
    CMTime halfTime = CMTimeMultiplyByFloat64(duration, 0.5);
    CMTime thirdQuarterTime = CMTimeMultiplyByFloat64(duration, 0.75);
    NSArray<NSValue *> *checkpoints = @[[NSValue valueWithCMTime:startTime],
                                        [NSValue valueWithCMTime:firstQuarterTime],
                                        [NSValue valueWithCMTime:halfTime],
                                        [NSValue valueWithCMTime:thirdQuarterTime]];

    __weak __typeof__(self) weakSelf = self;
    void (^observationHandler)(void) = ^void() {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }

        if (strongSelf.didFireStartEvent == NO) { // fire Start at first and once only
            strongSelf.didFireStartEvent = YES;
            [strongSelf.delegate videoPlayerViewDidStartVideo:strongSelf duration:durationInSeconds];
        }

        [strongSelf.delegate videoPlayerView:strongSelf
                   videoDidReachProgressTime:CMTimeGetSeconds(strongSelf.player.currentTime)
                                    duration:durationInSeconds];
    };

    // `addBoundaryTimeObserverForTimes` has undefined behavior with concurrent queue obtained
    // from `dispatch_get_global_queue`, thus use the main queue here since it's serial.
    self.boundaryTrackingTimeObserver = [self.player addBoundaryTimeObserverForTimes:checkpoints
                                                                               queue:dispatch_get_main_queue()
                                                                          usingBlock:observationHandler];
    self.endTimeObserverToken
    = [self.notificationCenter
       addObserverForName:AVPlayerItemDidPlayToEndTimeNotification
       object:self.player.currentItem
       queue:NSOperationQueue.mainQueue
       usingBlock:^(NSNotification *notification) {
        weakSelf.didPlayToEndTime = YES;
        [weakSelf.delegate videoPlayerViewDidCompleteVideo:weakSelf duration:durationInSeconds];
    }];

    self.audioSessionInterruptionObserverToken
    = [self.notificationCenter
       addObserverForName:AVAudioSessionInterruptionNotification
       object:nil
       queue:NSOperationQueue.mainQueue
       usingBlock:^(NSNotification *notification) {
        NSNumber *interruptionType = [notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey];
        if (weakSelf.isAutoPlayPauseEnabled
            && weakSelf.didPlayToEndTime == NO
            && interruptionType.unsignedIntegerValue == AVAudioSessionInterruptionTypeEnded) {
            // After an interruption (such as a phone call), we don't want the player to remain paused
            // because we don't offer a Play button, and potentially no Skip nor Close in some cases.
            [weakSelf play];
        }
    }];
}

/**
 Observe the times that the industry icon should show or hide.
 */
- (void)observeBoundaryTimeForIndustryIcons:(NSArray<MPVASTIndustryIcon *> *)industryIcons
                              videoDuration:(NSTimeInterval)videoDuration {
    if (industryIcons.count == 0) {
        return;
    }

    // guarantee the icons are shown in chronological order
    NSMutableArray<MPVASTIndustryIcon *> *sortedIndustryIcons = [[NSMutableArray alloc] initWithArray:industryIcons];
    [sortedIndustryIcons sortUsingComparator:^NSComparisonResult(MPVASTIndustryIcon *a, MPVASTIndustryIcon *b) {
        NSTimeInterval showTimeA = [a.offset timeIntervalForVideoWithDuration:videoDuration];
        NSTimeInterval showTimeB = [b.offset timeIntervalForVideoWithDuration:videoDuration];
        if (showTimeA < showTimeB) {
            return NSOrderedAscending;
        } else if (showTimeA > showTimeB) {
            return NSOrderedDescending;
        } else {
            return NSOrderedSame;
        }
    }];

    NSMutableArray<NSValue *> *showIconCheckpoints = [NSMutableArray new];
    NSMutableArray<NSValue *> *hideIconCheckpoints = [NSMutableArray new];

    for (MPVASTIndustryIcon *icon in sortedIndustryIcons) {
        NSTimeInterval timeToShowIcon = [icon.offset timeIntervalForVideoWithDuration:videoDuration];
        if (timeToShowIcon == 0) {
            timeToShowIcon = 0.01; // setting to 0 second won't work
        }
        CMTime showIconTime = CMTimeMakeWithSeconds(timeToShowIcon, kPreferredTimescale);
        [showIconCheckpoints addObject:[NSValue valueWithCMTime:showIconTime]];

        if (icon.duration > 0) { // duration is optional and can be 0
            CMTime hideIconTime = CMTimeMakeWithSeconds(timeToShowIcon + icon.duration, kPreferredTimescale);
            [hideIconCheckpoints addObject:[NSValue valueWithCMTime:hideIconTime]];
        }
    }

    __weak __typeof__(self) weakSelf = self;
    __block NSUInteger iconIndex = 0;
    void (^showIconHandler)(void) = ^void() {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf == nil || iconIndex >= sortedIndustryIcons.count) {
            return;
        }

        [strongSelf.delegate videoPlayerView:strongSelf showIndustryIcon:sortedIndustryIcons[iconIndex]];
        iconIndex++;
    };

    void (^hideIconHandler)(void) = ^void() {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        [strongSelf.delegate videoPlayerViewHideIndustryIcon:strongSelf];
    };

    // `addBoundaryTimeObserverForTimes` has undefined behavior with concurrent queue obtained
    // from `dispatch_get_global_queue`, thus use the main queue here since it's serial.
    self.industryIconShowTimeObserver = [self.player addBoundaryTimeObserverForTimes:showIconCheckpoints
                                                                               queue:dispatch_get_main_queue()
                                                                          usingBlock:showIconHandler];
    self.industryIconHideTimeObserver = [self.player addBoundaryTimeObserverForTimes:hideIconCheckpoints
                                                                               queue:dispatch_get_main_queue()
                                                                          usingBlock:hideIconHandler];
}

@end
