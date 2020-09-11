//
//  MPViewabilityAdapterAvid.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#if __has_include("MoPub.h")
#import "MoPub.h"
#import "MPLogging.h"
#endif

#import "MPViewabilityAdapterAvid.h"

#if __has_include("MoPub_Avid.h")
#import "MoPub_Avid.h"
#define __HAS_AVID_LIB_
#endif

@interface MPViewabilityAdapterAvid()
@property (nonatomic, readwrite) BOOL isTracking;

#ifdef __HAS_AVID_LIB_
@property (nonatomic, strong) MoPub_AbstractAvidAdSession * avidAdSession;

/**
 The key is an `MPVideoEvent`, and the value is the `NSValue` represenation of a selector from
 `MoPub_AvidVideoPlaybackListener`.
 */
@property (nonatomic, strong) NSDictionary<MPVideoEvent, NSValue *> *videoEventHandlerMap;
#endif
@end

@implementation MPViewabilityAdapterAvid

#pragma mark - MPViewabilityAdapter

- (void)startTracking {
#ifdef __HAS_AVID_LIB_
    // Only start tracking if:
    // 1. Avid is not already tracking
    // 2. Avid session is valid
    if (!self.isTracking && self.avidAdSession != nil) {
        [self.avidAdSession.avidDeferredAdSessionListener recordReadyEvent];
        self.isTracking = YES;
        MPLogInfo(@"IAS tracking started");
    }
#endif
}

- (void)stopTracking {
#ifdef __HAS_AVID_LIB_
    // Only stop tracking if:
    // 1. IAS is already tracking
    if (self.isTracking) {
        [self.avidAdSession endSession];
        if (self.avidAdSession) {
            MPLogInfo(@"IAS tracking stopped");
        }
    }

    // Mark IAS as not tracking
    self.isTracking = NO;
#endif
}

- (void)registerFriendlyObstructionView:(UIView *)view {
#ifdef __HAS_AVID_LIB_
    [self.avidAdSession registerFriendlyObstruction:view];
#endif
}

#pragma mark - MPViewabilityAdapterForWebView

- (instancetype)initWithWebView:(UIView *)webView isVideo:(BOOL)isVideo startTrackingImmediately:(BOOL)startTracking {
    if (self = [super init]) {
        _isTracking = NO;

#ifdef __HAS_AVID_LIB_
        MoPub_ExternalAvidAdSessionContext * avidAdSessionContext = [MoPub_ExternalAvidAdSessionContext contextWithPartnerVersion:[[MoPub sharedInstance] version] isDeferred:!startTracking];
        if (isVideo) {
            _avidAdSession = [MoPub_AvidAdSessionManager startAvidVideoAdSessionWithContext:avidAdSessionContext];
        }
        else {
            _avidAdSession = [MoPub_AvidAdSessionManager startAvidDisplayAdSessionWithContext:avidAdSessionContext];
        }

        [_avidAdSession registerAdView:webView];

        if (startTracking) {
            _isTracking = YES;
            MPLogInfo(@"IAS tracking started");
        }
#endif
    }

    return self;
}

#pragma mark - MPViewabilityAdapterForNativeVideoView

- (instancetype)initWithNativeVideoView:(UIView *)nativeVideoView startTrackingImmediately:(BOOL)startTracking {
    if (self = [super init]) {
        _isTracking = NO;

#ifdef __HAS_AVID_LIB_
        MoPub_ExternalAvidAdSessionContext *avidAdSessionContext
        = [MoPub_ExternalAvidAdSessionContext contextWithPartnerVersion:[[MoPub sharedInstance] version]
                                                             isDeferred:!startTracking];
        _avidAdSession = [MoPub_AvidAdSessionManager startAvidManagedVideoAdSessionWithContext:avidAdSessionContext];
        [_avidAdSession registerAdView:nativeVideoView];

        // note: not every `MPVideoEvent` has a corresponding `MoPub_AvidVideoPlaybackListener` call
        _videoEventHandlerMap = @{
            MPVideoEventClick: [NSValue valueWithPointer:@selector(recordAdClickThruEvent)],
            MPVideoEventClose: [NSValue valueWithPointer:@selector(recordAdUserCloseEvent)],
            MPVideoEventCloseLinear: [NSValue valueWithPointer:@selector(recordAdUserCloseEvent)],
            MPVideoEventComplete: [NSValue valueWithPointer:@selector(recordAdCompleteEvent)],
            MPVideoEventError: [NSValue valueWithPointer:@selector(recordAdErrorWithMessage:)],
            MPVideoEventExitFullScreen: [NSValue valueWithPointer:@selector(recordAdExitedFullscreenEvent)],
            MPVideoEventExpand: [NSValue valueWithPointer:@selector(recordAdExpandedChangeEvent)],
            MPVideoEventFirstQuartile: [NSValue valueWithPointer:@selector(recordAdVideoFirstQuartileEvent)],
            MPVideoEventFullScreen: [NSValue valueWithPointer:@selector(recordAdEnteredFullscreenEvent)],
            MPVideoEventImpression: [NSValue valueWithPointer:@selector(recordAdImpressionEvent)],
            MPVideoEventMidpoint: [NSValue valueWithPointer:@selector(recordAdVideoMidpointEvent)],
            MPVideoEventPause: [NSValue valueWithPointer:@selector(recordAdPausedEvent)],
            MPVideoEventResume: [NSValue valueWithPointer:@selector(recordAdPlayingEvent)],
            MPVideoEventSkip: [NSValue valueWithPointer:@selector(recordAdSkippedEvent)],
            MPVideoEventStart: [NSValue valueWithPointer:@selector(recordAdVideoStartEvent)],
            MPVideoEventThirdQuartile: [NSValue valueWithPointer:@selector(recordAdVideoThirdQuartileEvent)]
        };

        if (startTracking) {
            _isTracking = YES;
            MPLogInfo(@"IAS tracking started");
        }
#endif
    }

    return self;
}

- (void)trackNativeVideoEvent:(MPVideoEvent)event eventInfo:(NSDictionary<NSString *, id> *)eventInfo {
#ifdef __HAS_AVID_LIB_
    if ([self.avidAdSession isKindOfClass:[MoPub_AvidManagedVideoAdSession class]] == NO) {
        MPLogInfo(@"%s is called but the ad session is not for native video", __PRETTY_FUNCTION__);
        return;
    }

    MoPub_AvidManagedVideoAdSession *session = (MoPub_AvidManagedVideoAdSession *)self.avidAdSession;
    NSValue *selectorValue = self.videoEventHandlerMap[event];
    SEL selector = selectorValue.pointerValue;
    BOOL didTrack = NO;

    if (selector != nil) {
        NSString *selectorString = NSStringFromSelector(selector);
        if ([selectorString componentsSeparatedByString:@":"].count == 1) { // no argument
            [session.avidVideoPlaybackListener performSelector:selector];
            didTrack = YES;
        } else if ([selectorString componentsSeparatedByString:@":"].count == 2) { // one argument
            if (selector == @selector(MPVideoEventError)) {
                [session.avidVideoPlaybackListener performSelector:selector withObject:eventInfo[@"message"]];
                didTrack = YES;
            }
        }
    }

    if (didTrack == NO) {
        MPLogInfo(@"%s Unsupported tracking event %@", __PRETTY_FUNCTION__, event);
    }
#endif
}

@end
