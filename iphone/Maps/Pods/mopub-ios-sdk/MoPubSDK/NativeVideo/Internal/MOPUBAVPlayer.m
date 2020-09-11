//
//  MOPUBAVPlayer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBAVPlayer.h"
#import "MPLogging.h"
#import "MPReachabilityManager.h"
#import "MPTimer.h"
#import "MPCoreInstanceProvider.h"

static CGFloat const kAvPlayerTimerInterval = 0.1f;

static NSString * const MPAVPlayerItemLoadErrorTemplate = @"Loading player item at %@ failed.";

@interface MOPUBAVPlayer()

@property (nonatomic, weak, readonly) id<MOPUBAVPlayerDelegate> delegate;

@property (nonatomic, copy) NSURL *mediaURL;
@property (nonatomic) MPTimer *playbackTimer;
@property (nonatomic) CMTime lastContinuousPlaybackCMTime;
@property (nonatomic) BOOL playbackDidStall;

@end

@implementation MOPUBAVPlayer

- (id)initWithDelegate:(id<MOPUBAVPlayerDelegate>)delegate playerItem:(AVPlayerItem *)playerItem
{
    if (playerItem && delegate) {
        self = [super initWithPlayerItem:playerItem];
        if (self) {
            _delegate = delegate;

            // AVPlayer KVO doesn't handle disconnect/reconnect case.
            // Reachability is used to detect network drop and reconnect.
            [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(checkNetworkStatus:) name:kMPReachabilityChangedNotification object:nil];
            [MPReachabilityManager.sharedManager startMonitoring];
        }
        return self;
    } else {
        return nil;
    }
}

- (void)dealloc
{
    [self dispose];
}

#pragma mark - controls of AVPlayer

- (void)play
{
    [super play];
    [self startTimeObserver];
    MPLogDebug(@"start playback");
}

- (void)pause
{
    [super pause];
    [self stopTimeObserver];
    MPLogDebug(@"playback paused");
}

- (void)setMuted:(BOOL)muted
{
    if ([[self superclass] instancesRespondToSelector:@selector(setMuted:)]) {
        [super setMuted:muted];
    } else {
        if (muted) {
            [self setAudioVolume:0];
        } else {
            [self setAudioVolume:1];
        }
    }
}

// iOS 6 doesn't have muted for avPlayerItem. Use volume to control mute/unmute
- (void)setAudioVolume:(float)volume
{
    NSArray *audioTracks = [self.currentItem.asset tracksWithMediaType:AVMediaTypeAudio];
    NSMutableArray *allAudioParams = [NSMutableArray array];
    for (AVAssetTrack *track in audioTracks) {
        AVMutableAudioMixInputParameters *audioInputParams = [AVMutableAudioMixInputParameters audioMixInputParameters];
        [audioInputParams setVolume:volume atTime:kCMTimeZero];
        [audioInputParams setTrackID:[track trackID]];
        [allAudioParams addObject:audioInputParams];
    }
    AVMutableAudioMix *audioMix = [AVMutableAudioMix audioMix];
    [audioMix setInputParameters:allAudioParams];
    [self.currentItem setAudioMix:audioMix];
}

#pragma mark - Timer

- (void)startTimeObserver
{
    // Use custom timer to check for playback time changes and stall detection, since there are bugs
    // in the AVPlayer time observing API that can cause crashes. Also, the AVPlayerItem stall notification
    // does not always report accurately.
    if (_playbackTimer == nil) {
        // Add timer to main run loop with common modes to allow the timer to tick while user is scrolling.
        _playbackTimer = [MPTimer timerWithTimeInterval:kAvPlayerTimerInterval
                                                 target:self
                                               selector:@selector(timerTick)
                                                repeats:YES
                                            runLoopMode:NSRunLoopCommonModes];
        [_playbackTimer scheduleNow];
        _lastContinuousPlaybackCMTime = kCMTimeZero;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playbackDidFinish) name:AVPlayerItemDidPlayToEndTimeNotification object:self.currentItem];
    } else {
        [_playbackTimer resume];
    }
}

- (void)timerTick
{
    if (!self.currentItem || self.currentItem.error != nil) {
        [self stopTimeObserver];
        NSError *error = nil;
        NSString *errorMessage = nil;
        if (self.currentItem) {
            error = self.currentItem.error;
            errorMessage = self.currentItem.error.description ?: self.currentItem.errorLog.description;
        } else {
            errorMessage = [NSString stringWithFormat:MPAVPlayerItemLoadErrorTemplate, self.mediaURL];
        }

        if ([self.delegate respondsToSelector:@selector(avPlayer:didError:withMessage:)]) {
            [self.delegate avPlayer:self didError:error withMessage:errorMessage];
        }
        MPLogInfo(@"avplayer experienced error: %@", errorMessage);
    } else {
        CMTime currentCMTime = self.currentTime;
        int32_t result = CMTimeCompare(currentCMTime, self.lastContinuousPlaybackCMTime);
        // finished or stalled
        if (result == 0) {
            NSTimeInterval duration = self.currentItemDuration;
            NSTimeInterval currentPlaybackTime = self.currentPlaybackTime;
            if (!isnan(duration) && !isnan(currentPlaybackTime) && duration > 0 && currentPlaybackTime > 0) {
                [self avPlayerDidStall];
            }
        } else {
            self.lastContinuousPlaybackCMTime = currentCMTime;
            if (result > 0) {
                NSTimeInterval currentPlaybackTime = self.currentPlaybackTime;
                if (!isnan(currentPlaybackTime) && isfinite(currentPlaybackTime)) {
                    // There are bugs in AVPlayer that causes the currentTime to be negative
                    if (currentPlaybackTime < 0) {
                        currentPlaybackTime = 0;
                    }
                    [self avPlayer:self playbackTimeDidProgress:currentPlaybackTime];
                }
            }
        }

    }
}

- (void)stopTimeObserver
{
    [_playbackTimer pause];
    MPLogDebug(@"AVPlayer timer stopped");
}

#pragma mark - disconnect/reconnect handling
- (void)checkNetworkStatus:(NSNotification *)notice
{
    MPNetworkStatus remoteHostStatus = MPReachabilityManager.sharedManager.currentStatus;

    if (remoteHostStatus == MPNotReachable) {
        if (!self.rate) {
            [self pause];
            if ([self.delegate respondsToSelector:@selector(avPlayerDidStall:)]) {
                [self.delegate avPlayerDidStall:self];
            }
        }
    } else {
        if (!self.rate) {
            [self play];
        }
    }
}

#pragma mark - avPlayer state changes

- (void)avPlayer:(MOPUBAVPlayer *)player playbackTimeDidProgress:(NSTimeInterval)currentPlaybackTime
{
    if (self.playbackDidStall) {
        self.playbackDidStall = NO;
        if ([self.delegate respondsToSelector:@selector(avPlayerDidRecoverFromStall:)]) {
            [self.delegate avPlayerDidRecoverFromStall:self];
        }
    }

    if ([self.delegate respondsToSelector:@selector(avPlayer:playbackTimeDidProgress:)]) {
        [self.delegate avPlayer:self playbackTimeDidProgress:currentPlaybackTime];
    }
}

- (void)avPlayerDidStall
{
    // Only call delegate methods once per stall cycle.
    if (!self.playbackDidStall && [self.delegate respondsToSelector:@selector(avPlayerDidStall:)]) {
        [self.delegate avPlayerDidStall:self];
    }
    self.playbackDidStall = YES;
}

- (void)playbackDidFinish
{
    // Make sure we stop time observing once we know we've done playing.
    [self stopTimeObserver];
    if ([self.delegate respondsToSelector:@selector(avPlayerDidFinishPlayback:)]) {
        [self.delegate avPlayerDidFinishPlayback:self];
    }
    MPLogDebug(@"playback finished");
}

- (void)dispose
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [self stopTimeObserver];
    [MPReachabilityManager.sharedManager stopMonitoring];
    if (_playbackTimer) {
        [_playbackTimer invalidate];
        _playbackTimer = nil;
    }

    // Cancel preroll after time observer is removed,
    // otherwise an NSInternalInconsistencyException may be thrown and crash on
    // [AVCMNotificationDispatcher _copyAndRemoveListenerAndCallbackForWeakReferenceToListener:callback:name:object:],
    // depends on timing.
    [self cancelPendingPrerolls];
}


#pragma mark - getter

- (NSTimeInterval)currentItemDuration
{
    NSTimeInterval duration = CMTimeGetSeconds(self.currentItem.duration);
    return (isfinite(duration)) ? duration : NAN;
}

- (NSTimeInterval)currentPlaybackTime
{
    NSTimeInterval currentTime = CMTimeGetSeconds(self.currentTime);
    return (isfinite(currentTime)) ? currentTime : NAN;
}

@end
