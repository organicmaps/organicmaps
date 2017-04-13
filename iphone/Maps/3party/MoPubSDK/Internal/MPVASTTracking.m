//
//  MPVASTTracking.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBNativeVideoImpressionAgent.h"
#import "MPAnalyticsTracker.h"
#import "MPCoreInstanceProvider.h"
#import "MPLogging.h"
#import "MPVASTMacroProcessor.h"
#import "MPVASTTracking.h"
#import "MPVideoConfig.h"

// Do not fire the start tracker until this time has been reached in the video
static const NSInteger kStartTrackerTime = 0;

@interface VASTTrackingURL : NSObject

@property (nonatomic, copy) NSURL *url;
@property (nonatomic) MPVASTDurationOffset *progressOffset;

@end

@implementation VASTTrackingURL

@end

@interface VASTEventTracker : NSObject

@property (nonatomic, assign) BOOL trackersFired;
@property (nonatomic) NSArray *trackingEvents; // NSArray<VASTTrackingURL>

@end

@implementation VASTEventTracker

+ (VASTEventTracker *)eventTrackerWithMPVastTrackingEvents:(NSArray *)events
{
    VASTEventTracker *result = [[VASTEventTracker alloc] init];
    NSMutableArray *trackingEvents = [NSMutableArray array];
    for (MPVASTTrackingEvent *event in events) {
        VASTTrackingURL *tracker = [[VASTTrackingURL alloc] init];
        tracker.url = event.URL;
        tracker.progressOffset = event.progressOffset;
        [trackingEvents addObject:tracker];
    }

    result.trackingEvents = trackingEvents;

    return result;
}

+ (VASTEventTracker *)eventTrackerWithURLs:(NSArray *)urls
{
    VASTEventTracker *result = [[VASTEventTracker alloc] init];
    NSMutableArray *trackingEvents = [NSMutableArray array];
    for (NSURL *url in urls) {
        VASTTrackingURL *tracker = [[VASTTrackingURL alloc] init];
        tracker.url = url;
        tracker.progressOffset = nil;
        [trackingEvents addObject:tracker];
    }

    result.trackingEvents = trackingEvents;

    return result;
}

@end

@interface MPVASTTracking()

@property (nonatomic) VASTEventTracker *errorTracker;
@property (nonatomic) VASTEventTracker *impressionTracker;
@property (nonatomic) VASTEventTracker *clickTracker;
@property (nonatomic) VASTEventTracker *customViewabilityTracker;

@property (nonatomic) VASTEventTracker *startTracker;
@property (nonatomic) VASTEventTracker *firstQuartileTracker;
@property (nonatomic) VASTEventTracker *midPointTracker;
@property (nonatomic) VASTEventTracker *thirdQuartileTracker;
@property (nonatomic) VASTEventTracker *completionTracker;

@property (nonatomic) NSMutableArray *variableProgressTrackers; //NSMutableArray<VASTEventTracker>

@property (nonatomic) VASTEventTracker *muteTracker;
@property (nonatomic) VASTEventTracker *unmuteTracker;
@property (nonatomic) VASTEventTracker *pauseTracker;
@property (nonatomic) VASTEventTracker *rewindTracker;
@property (nonatomic) VASTEventTracker *resumeTracker;
@property (nonatomic) VASTEventTracker *fullscreenTracker;
@property (nonatomic) VASTEventTracker *exitFullscreenTracker;
@property (nonatomic) VASTEventTracker *expandTracker;
@property (nonatomic) VASTEventTracker *collapseTracker;

@property (nonatomic) MOPUBNativeVideoImpressionAgent *customViewabilityTrackingAgent;

@end

@implementation MPVASTTracking

- (instancetype)initWithMPVideoConfig:(MPVideoConfig *)videoConfig videoView:(UIView *)videoView;
{
    self = [super init];
    if (self) {
        _videoConfig = videoConfig;
        _videoDuration = -1;

        _errorTracker = [VASTEventTracker eventTrackerWithURLs:_videoConfig.errorURLs];
        _impressionTracker = [VASTEventTracker eventTrackerWithURLs:_videoConfig.impressionURLs];
        _clickTracker = [VASTEventTracker eventTrackerWithURLs:_videoConfig.clickTrackingURLs];

        _startTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.startTrackers];
        _firstQuartileTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.firstQuartileTrackers];
        _midPointTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.midpointTrackers];
        _thirdQuartileTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.thirdQuartileTrackers];
        _completionTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.completionTrackers];

        _variableProgressTrackers = [NSMutableArray array];
        for (MPVASTTrackingEvent *event in _videoConfig.otherProgressTrackers) {
            [_variableProgressTrackers addObject:[VASTEventTracker eventTrackerWithMPVastTrackingEvents:[NSArray arrayWithObject:event]]];
        }

        _muteTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.muteTrackers];
        _unmuteTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.unmuteTrackers];
        _pauseTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.pauseTrackers];
        _rewindTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.rewindTrackers];
        _resumeTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.resumeTrackers];
        _fullscreenTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.fullscreenTrackers];
        _exitFullscreenTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.exitFullscreenTrackers];
        _expandTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.expandTrackers];
        _collapseTracker = [VASTEventTracker eventTrackerWithMPVastTrackingEvents:_videoConfig.collapseTrackers];

        if (_videoConfig.viewabilityTrackingURL) {
            _customViewabilityTracker = [VASTEventTracker eventTrackerWithURLs:[NSArray arrayWithObject:_videoConfig.viewabilityTrackingURL]];
            _customViewabilityTrackingAgent = [[MOPUBNativeVideoImpressionAgent alloc] initWithVideoView:videoView requiredVisibilityPercentage:videoConfig.minimumFractionOfVideoVisible requiredPlaybackDuration:videoConfig.minimumViewabilityTimeInterval];
        }
    }
    return self;
}

- (void)handleVideoEvent:(MPVideoEventType)videoEventType videoTimeOffset:(NSTimeInterval)timeOffset
{
    if (self.videoConfig && (self.videoDuration > 0 || videoEventType == MPVideoEventTypeError)) {
        if (videoEventType == MPVideoEventTypeTimeUpdate) {
            [self handleProgressTrackers:timeOffset];
        } else {
            VASTEventTracker *eventTrackerToFire;
            switch (videoEventType) {
                case MPVideoEventTypeMuted:
                    eventTrackerToFire = self.muteTracker;
                    break;
                case MPVideoEventTypeUnmuted:
                    eventTrackerToFire = self.unmuteTracker;
                    break;
                case MPVideoEventTypePause:
                    eventTrackerToFire = self.pauseTracker;
                    break;
                case MPVideoEventTypeResume:
                    eventTrackerToFire = self.resumeTracker;
                    break;
                case MPVideoEventTypeFullScreen:
                    eventTrackerToFire = self.fullscreenTracker;
                    break;
                case MPVideoEventTypeExitFullScreen:
                    eventTrackerToFire = self.exitFullscreenTracker;
                    break;
                case MPVideoEventTypeExpand:
                    eventTrackerToFire = self.expandTracker;
                    break;
                case MPVideoEventTypeCollapse:
                    eventTrackerToFire = self.collapseTracker;
                    break;
                case MPVideoEventTypeError:
                    eventTrackerToFire = self.errorTracker;
                    break;
                case MPVideoEventTypeImpression:
                    if (!self.impressionTracker.trackersFired) {
                        eventTrackerToFire = self.impressionTracker;
                    }
                    break;
                case MPVideoEventTypeClick:
                    if (!self.clickTracker.trackersFired) {
                        eventTrackerToFire = self.clickTracker;
                    }
                    break;
                case MPVideoEventTypeCompleted:
                    if (!self.completionTracker.trackersFired) {
                        eventTrackerToFire = self.completionTracker;
                    }
                    break;
                default:
                    eventTrackerToFire = nil;
            }
            // Only fire event trackers after the video has started playing
            if (eventTrackerToFire && (self.startTracker.trackersFired || videoEventType == MPVideoEventTypeError)) {
                [self cleanAndSendTrackingEvents:eventTrackerToFire timeOffset:timeOffset];
            }
        }
    }
}

- (void)handleProgressTrackers:(NSTimeInterval)timeOffset
{
    if (timeOffset >= kStartTrackerTime && !self.startTracker.trackersFired) {
        [self cleanAndSendTrackingEvents:self.startTracker timeOffset:timeOffset];
    }

    if ((0.75 * self.videoDuration) <= timeOffset && !self.thirdQuartileTracker.trackersFired) {
        [self cleanAndSendTrackingEvents:self.thirdQuartileTracker timeOffset:timeOffset];
    }

    if ((0.50 * self.videoDuration) <= timeOffset && !self.midPointTracker.trackersFired) {
        [self cleanAndSendTrackingEvents:self.midPointTracker timeOffset:timeOffset];
    }

    if ((0.25 * self.videoDuration) <= timeOffset && !self.firstQuartileTracker.trackersFired) {
        [self cleanAndSendTrackingEvents:self.firstQuartileTracker timeOffset:timeOffset];
    }

    for (VASTEventTracker *progressTracker in self.variableProgressTrackers) {
        VASTTrackingURL *progressTrackingURL = progressTracker.trackingEvents[0]; // there's only one
        if (!progressTracker.trackersFired && [progressTrackingURL.progressOffset timeIntervalForVideoWithDuration:self.videoDuration] <= timeOffset) {
            [self cleanAndSendTrackingEvents:progressTracker timeOffset:timeOffset];
        }
    }

    if (self.customViewabilityTracker && !self.customViewabilityTracker.trackersFired &&
        [self.customViewabilityTrackingAgent shouldTrackImpressionWithCurrentPlaybackTime:timeOffset]) {
        [self cleanAndSendTrackingEvents:self.customViewabilityTracker timeOffset:timeOffset];
    }
}

- (void)cleanAndSendTrackingEvents:(VASTEventTracker *)vastEventTracker timeOffset:(NSTimeInterval)timeOffset
{
    if (vastEventTracker && [vastEventTracker.trackingEvents count]) {
        NSMutableArray *cleanedTrackingURLs = [NSMutableArray array];
        for (VASTTrackingURL *vastTrackingURL in vastEventTracker.trackingEvents) {
            [cleanedTrackingURLs addObject:[MPVASTMacroProcessor macroExpandedURLForURL:vastTrackingURL.url errorCode:nil videoTimeOffset:timeOffset videoAssetURL:self.videoConfig.mediaURL]];
        }
        [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] sendTrackingRequestForURLs:cleanedTrackingURLs];
    }
    vastEventTracker.trackersFired = YES;
}

- (void)handleNewVideoView:(UIView *)videoView
{
    self.customViewabilityTrackingAgent = [[MOPUBNativeVideoImpressionAgent alloc] initWithVideoView:videoView requiredVisibilityPercentage:self.videoConfig.minimumFractionOfVideoVisible requiredPlaybackDuration:self.videoConfig.minimumViewabilityTimeInterval];
}

@end
