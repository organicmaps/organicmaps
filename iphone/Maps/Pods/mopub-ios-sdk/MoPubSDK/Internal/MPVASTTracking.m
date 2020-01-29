//
//  MPVASTTracking.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAnalyticsTracker.h"
#import "MPVASTMacroProcessor.h"
#import "MPVASTTracking.h"

static dispatch_once_t dispatchOnceToken; // for `oneOffEventTypes`
static NSSet<MPVideoEvent> *oneOffEventTypes;

@interface MPVASTTracking()

@property (nonatomic, strong) MPVideoConfig *videoConfig;
@property (nonatomic, strong) NSURL *videoURL;
@property (nonatomic, strong) id<MPAnalyticsTracker> analyticsTracker;

/**
 The key is a @c MPVideoEvent string, and the value is a set of fired @c MPVASTTrackingEvent of the same type.

 Each event could associate to multiple tracking URL's and thus multiple `MPVASTTrackingEvent`s. For
 most events except @c MPVideoEventProgress, all the tracking URL's are sent at the same time. For
 the @c MPVideoEventProgress case, typically different URL's are supposed to be sent at different
 times (at 0s, 5s, 10s, and so on), and thus we have to use a dictionary with @c MPVideoEvent as key
 and @c NSMutableSet<MPVASTTrackingEvent *> as the value to keep track of the fired status, instead
 of just a simple @c NSMutableSet<MPVideoEvent> that cannot differentiate the @c MPVideoEventProgress
 events of different play times.
 */
@property (nonatomic, strong) NSMutableDictionary<MPVideoEvent, NSMutableSet<MPVASTTrackingEvent *> *> *firedTable;

@end

@implementation MPVASTTracking

- (instancetype)initWithVideoConfig:(MPVideoConfig *)videoConfig videoURL:(NSURL *)videoURL {
    self = [super init];
    if (self) {
        _videoConfig = videoConfig;
        _videoURL = videoURL;
        _analyticsTracker = [MPAnalyticsTracker sharedTracker];
        _firedTable = [NSMutableDictionary new];

        dispatch_once(&dispatchOnceToken, ^{
            oneOffEventTypes = [NSSet setWithObjects:
                                MPVideoEventClick,
                                MPVideoEventCloseLinear,
                                MPVideoEventComplete,
                                MPVideoEventCreativeView,
                                MPVideoEventFirstQuartile,
                                MPVideoEventImpression,
                                MPVideoEventMidpoint,
                                MPVideoEventProgress,
                                MPVideoEventSkip,
                                MPVideoEventStart,
                                MPVideoEventThirdQuartile,
                                nil];
        });
    }
    return self;
}

- (void)handleVideoEvent:(MPVideoEvent)videoEvent videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    if ([oneOffEventTypes containsObject:videoEvent]
        && self.firedTable[videoEvent] != nil) {
        return; // do not fire more than once
    }

    if (videoEvent == MPVideoEventError) {
        [self handleVASTError:MPVASTErrorCannotPlayMedia videoTimeOffset:videoTimeOffset];
        return;
    }

    if (self.videoConfig == nil && [videoEvent isEqualToString:MPVideoEventError] == NO) {
        return; // only allow `videoConfig` to be nil for error event
    }

    if ([videoEvent isEqualToString:MPVideoEventProgress]) {
        return; // call `handleVideoProgressEvent:videoDuration:` instead
    }

    NSMutableSet<MPVASTTrackingEvent *> *firedEvents = [NSMutableSet new];
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    for (MPVASTTrackingEvent *event in [self.videoConfig trackingEventsForKey:videoEvent]) {
        [urls addObject:event.URL];
        [firedEvents addObject:event];
    }

    if (urls.count > 0) {
        [self processAndSendURLs:urls videoTimeOffset:videoTimeOffset];
    }
    self.firedTable[videoEvent] = firedEvents;
}

- (void)handleVideoProgressEvent:(NSTimeInterval)videoTimeOffset videoDuration:(NSTimeInterval)videoDuration {
    if (videoTimeOffset < 0 || videoDuration <= 0) {
        return;
    }

    if (self.firedTable[MPVideoEventStart] == nil) {
        [self handleVideoEvent:MPVideoEventStart videoTimeOffset:videoTimeOffset];
    }
    if ((0.25 * videoDuration) <= videoTimeOffset
        && self.firedTable[MPVideoEventFirstQuartile] == nil) {
        [self handleVideoEvent:MPVideoEventFirstQuartile videoTimeOffset:videoTimeOffset];
    }
    if ((0.50 * videoDuration) <= videoTimeOffset
        && self.firedTable[MPVideoEventMidpoint] == nil) {
        [self handleVideoEvent:MPVideoEventMidpoint videoTimeOffset:videoTimeOffset];
    }
    if ((0.75 * videoDuration) <= videoTimeOffset
        && self.firedTable[MPVideoEventThirdQuartile] == nil) {
        [self handleVideoEvent:MPVideoEventThirdQuartile videoTimeOffset:videoTimeOffset];
    }
    // The Complete event is not handled in this method intentionally. Please see header comments.

    // `MPVideoEventProgress` specific handling: do not use `handleVideoEvent:videoTimeOffset:`
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    NSMutableSet<MPVASTTrackingEvent *> *firedProgressEvents = self.firedTable[MPVideoEventProgress];
    for (MPVASTTrackingEvent *event in [self.videoConfig trackingEventsForKey:MPVideoEventProgress]) {
        if ([firedProgressEvents containsObject:event] == NO
            && [event.progressOffset timeIntervalForVideoWithDuration:videoDuration] <= videoTimeOffset) {
            [urls addObject:event.URL];

            if (firedProgressEvents == nil) {
                firedProgressEvents = [NSMutableSet new];
            }
            [firedProgressEvents addObject:event];
        }
    }

    if (urls.count > 0) {
        [self processAndSendURLs:urls videoTimeOffset:videoTimeOffset];
    }
    self.firedTable[MPVideoEventProgress] = firedProgressEvents;
}

- (void)handleVASTError:(MPVASTError)error videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    for (MPVASTTrackingEvent *event in [self.videoConfig trackingEventsForKey:MPVideoEventError]) {
        [urls addObject:event.URL];
    }
    [self processAndSendURLs:urls videoTimeOffset:videoTimeOffset];
}

#pragma mark - Private

- (void)processAndSendURLs:(NSSet<NSURL *> *)urls
           videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    if ([urls count] == 0) {
        return;
    }

    NSMutableArray<NSURL *> *processedURLs = [NSMutableArray new];
    for (NSURL *url in urls) {
        [processedURLs addObject:[MPVASTMacroProcessor
                                  macroExpandedURLForURL:url
                                  errorCode:nil
                                  videoTimeOffset:videoTimeOffset
                                  videoAssetURL:self.videoURL]];
    }
    [self.analyticsTracker sendTrackingRequestForURLs:processedURLs];
}

@end
