//
//  MPVASTTracking.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAnalyticsTracker.h"
#import "MPVASTMacroProcessor.h"
#import "MPVASTTracking.h"
#import "MPViewabilityTracker.h"

static dispatch_once_t dispatchOnceToken; // for `oneOffEventTypes`
static NSSet<MPVideoEvent> *oneOffEventTypes;

@interface MPVASTTracking()

@property (nonatomic, strong) MPVideoConfig *videoConfig;
@property (nonatomic, strong) NSURL *videoURL;
@property (nonatomic, strong) id<MPAnalyticsTracker> analyticsTracker; // Note: only send URL's with `uniquelySendURLs`
@property (nonatomic, strong) MPViewabilityTracker *viewabilityTracker;

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
@property (nonatomic, strong) NSMutableDictionary<MPVideoEvent, NSMutableSet<MPVASTTrackingEvent *> *> *firedEventTable;

/**
 @c firedEventTable is good for tracking events targetting the video. However, each VAST ad might
 have multiple industry icons and companions ads, and we cannot reuse one event name across different
 industry icons / companions ads. This set is to make sure each URL is only sent once.
 */
@property (nonatomic, strong) NSMutableSet<NSURL *> *sentURLs;

@end

@implementation MPVASTTracking

- (instancetype)initWithVideoConfig:(MPVideoConfig *)videoConfig videoURL:(NSURL *)videoURL {
    self = [super init];
    if (self) {
        _videoConfig = videoConfig;
        _videoURL = videoURL;
        _analyticsTracker = [MPAnalyticsTracker sharedTracker];
        _firedEventTable = [NSMutableDictionary new];
        _sentURLs = [NSMutableSet new];

        dispatch_once(&dispatchOnceToken, ^{
            oneOffEventTypes = [NSSet setWithObjects:
                                MPVideoEventClick,
                                MPVideoEventClose,
                                MPVideoEventCloseLinear,
                                MPVideoEventCompanionAdView,
                                MPVideoEventCompanionAdClick,
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

- (void)registerVideoViewForViewabilityTracking:(UIView *)videoView {
    self.viewabilityTracker = [[MPViewabilityTracker alloc] initWithNativeVideoView:videoView
                                                           startTrackingImmediately:YES];
}

- (void)stopViewabilityTracking {
    [self.viewabilityTracker stopTracking];
}

- (void)uniquelySendURLs:(NSArray<NSURL *> *)urls {
    NSMutableSet *urlsToSend = [NSMutableSet new];
    for (NSURL *url in urls) {
        if (![self.sentURLs containsObject:url]) {
            [urlsToSend addObject:url];
        }
    }
    [self.analyticsTracker sendTrackingRequestForURLs:urlsToSend.allObjects];
    [self.sentURLs unionSet:urlsToSend];
}

- (void)handleVideoEvent:(MPVideoEvent)videoEvent videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    BOOL disallowPreviouslySentURLs = [oneOffEventTypes containsObject:videoEvent];

    if (disallowPreviouslySentURLs
        && self.firedEventTable[videoEvent] != nil) {
        return; // do not fire more than once
    }

    if ([videoEvent isEqualToString:MPVideoEventError]) {
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
        [self processAndSendURLs:urls
      disallowPreviouslySentURLs:disallowPreviouslySentURLs
                       errorCode:nil
                 videoTimeOffset:videoTimeOffset];
    }
    self.firedEventTable[videoEvent] = firedEvents;

    [self.viewabilityTracker trackNativeVideoEvent:videoEvent eventInfo:nil];
}

- (void)handleVideoProgressEvent:(NSTimeInterval)videoTimeOffset videoDuration:(NSTimeInterval)videoDuration {
    if (videoTimeOffset < 0 || videoDuration <= 0) {
        return;
    }

    if ((0.25 * videoDuration) <= videoTimeOffset
        && self.firedEventTable[MPVideoEventFirstQuartile] == nil) {
        [self handleVideoEvent:MPVideoEventFirstQuartile videoTimeOffset:videoTimeOffset];
    }
    if ((0.50 * videoDuration) <= videoTimeOffset
        && self.firedEventTable[MPVideoEventMidpoint] == nil) {
        [self handleVideoEvent:MPVideoEventMidpoint videoTimeOffset:videoTimeOffset];
    }
    if ((0.75 * videoDuration) <= videoTimeOffset
        && self.firedEventTable[MPVideoEventThirdQuartile] == nil) {
        [self handleVideoEvent:MPVideoEventThirdQuartile videoTimeOffset:videoTimeOffset];
    }
    // The Complete event is not handled in this method intentionally. Please see header comments.

    // `MPVideoEventProgress` specific handling: do not use `handleVideoEvent:videoTimeOffset:`
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    NSMutableSet<MPVASTTrackingEvent *> *firedProgressEvents = self.firedEventTable[MPVideoEventProgress];
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
        [self processAndSendURLs:urls
      disallowPreviouslySentURLs:YES
                       errorCode:nil
                 videoTimeOffset:videoTimeOffset];
    }
    self.firedEventTable[MPVideoEventProgress] = firedProgressEvents;
}

- (void)handleVASTError:(MPVASTError)error videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    for (MPVASTTrackingEvent *event in [self.videoConfig trackingEventsForKey:MPVideoEventError]) {
        [urls addObject:event.URL];
    }
    [self processAndSendURLs:urls
  disallowPreviouslySentURLs:NO
                   errorCode:[NSString stringWithFormat:@"%lu", (unsigned long)error]
             videoTimeOffset:videoTimeOffset];

    [self.viewabilityTracker trackNativeVideoEvent:MPVideoEventError
                                         eventInfo:@{@"message": [self stringFromVASTError:error]}];
}

#pragma mark - Private

/**
 @c errorCode is the @c NSString representation of @c `MPVASTError`.
 */
- (void)processAndSendURLs:(NSSet<NSURL *> *)urls
disallowPreviouslySentURLs:(BOOL)disallowPreviouslySentURLs
                 errorCode:(NSString *)errorCode
           videoTimeOffset:(NSTimeInterval)videoTimeOffset {
    if ([urls count] == 0) {
        return;
    }

    NSMutableSet<NSURL *> *processedURLs = [NSMutableSet new];
    for (NSURL *url in urls) {
        [processedURLs addObject:[MPVASTMacroProcessor
                                  macroExpandedURLForURL:url
                                  errorCode:errorCode
                                  videoTimeOffset:videoTimeOffset
                                  videoAssetURL:self.videoURL]];
    }

    if (disallowPreviouslySentURLs) {
        [self uniquelySendURLs:urls.allObjects];
    }
    else {
        [self.analyticsTracker sendTrackingRequestForURLs:urls.allObjects];
    }
}

- (NSString *)stringFromVASTError:(MPVASTError)error {
    // return the message from `MPVASTError` comments
    switch (error) {
        case MPVASTErrorXMLParseFailure:
            return @"XML parsing error.";
        case MPVASTErrorCannotPlayMedia:
            return @"Trafficking error. Media player received an Ad type that it was not expecting and/or cannot play.";
        case MPVASTErrorExceededMaximumWrapperDepth:
            return @"Wrapper limit reached, as defined by the media player.";
        case MPVASTErrorNoVASTResponseAfterOneOrMoreWrappers:
            return @"No VAST response after one or more Wrappers.";
        case MPVASTErrorFailedToDisplayAdFromInlineResponse:
            return @"InLine response returned ad unit that failed to result in ad display within defined time limit.";
        case MPVASTErrorUnableToFindLinearAdOrMediaFileFromURI:
            return @"General Linear error. Media player is unable to display the Linear Ad.";
        case MPVASTErrorTimeoutOfMediaFileURI:
            return @"File not found. Unable to find Linear/MediaFile from URI.";
        case MPVASTErrorMezzanineIsBeingProccessed:
            return @"Mezzanine is in the process of being downloaded for the first time.";
        case MPVASTErrorGeneralCompanionAdsError:
            return @"General CompanionAds error.";
        case MPVASTErrorUndefined:
            return @"Undefined Error.";
    }
}

@end
