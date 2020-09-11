//
//  MPVideoEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 To learn more about these events, please see VAST documentation:
    https://www.iab.com/wp-content/uploads/2015/06/VASTv3_0.pdf

 Note: for `MPVideoEventCloseLinear`("closeLinear"): the user clicked the close button on the
 creative. The name of this event distinguishes it from the existing “close” event described in the
 2008 IAB Digital Video In-Stream Ad Metrics Definitions, which defines the “close” metric as
 applying to non-linear ads only. The "closeLinear" event extends the “close” event for use in Linear
 creative.

 Since we don't know whether none, either, or both "close" or "closeLinear" are returned by the
 creative, we should look for both trackers when the user closes the video.
 */
typedef NSString * MPVideoEvent;

// keep this list sorted alphabetically
extern MPVideoEvent const MPVideoEventClick;
extern MPVideoEvent const MPVideoEventClose; // see note above about `MPVideoEventCloseLinear`
extern MPVideoEvent const MPVideoEventCloseLinear; // see note above about `MPVideoEventClose`
extern MPVideoEvent const MPVideoEventCollapse;
extern MPVideoEvent const MPVideoEventCompanionAdClick; // MoPub-specific tracking event
extern MPVideoEvent const MPVideoEventCompanionAdView;  // MoPub-specific tracking event
extern MPVideoEvent const MPVideoEventComplete;
extern MPVideoEvent const MPVideoEventCreativeView;
extern MPVideoEvent const MPVideoEventError;
extern MPVideoEvent const MPVideoEventExitFullScreen;
extern MPVideoEvent const MPVideoEventExpand;
extern MPVideoEvent const MPVideoEventFirstQuartile;
extern MPVideoEvent const MPVideoEventFullScreen;
extern MPVideoEvent const MPVideoEventImpression;
extern MPVideoEvent const MPVideoEventIndustryIconClick; // MoPub-specific tracking event
extern MPVideoEvent const MPVideoEventIndustryIconView; // MoPub-specific tracking event
extern MPVideoEvent const MPVideoEventMidpoint;
extern MPVideoEvent const MPVideoEventMute;
extern MPVideoEvent const MPVideoEventPause;
extern MPVideoEvent const MPVideoEventProgress;
extern MPVideoEvent const MPVideoEventResume;
extern MPVideoEvent const MPVideoEventSkip;
extern MPVideoEvent const MPVideoEventStart;
extern MPVideoEvent const MPVideoEventThirdQuartile;
extern MPVideoEvent const MPVideoEventUnmute;

/**
 Provides class-level support for `MPVideoEvent` processing.
 */
@interface MPVideoEvents : NSObject

/**
 All available and supported `MPVideoEvent` types.
 */
@property (nonatomic, class, strong, readonly) NSSet<MPVideoEvent> *supported;

/**
 Queries if the inputted event string is a valid supported `MPVideoEvent` type.
 @param event Video event candidate string.
 @return `true` if valid; `false` otherwise.
 */
+ (BOOL)isSupportedEvent:(NSString *)event;

@end

NS_ASSUME_NONNULL_END
