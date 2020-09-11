//
//  MPVideoEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVideoEvent.h"

// keep this list sorted alphabetically
MPVideoEvent const MPVideoEventClick = @"click";
MPVideoEvent const MPVideoEventClose = @"close";
MPVideoEvent const MPVideoEventCloseLinear = @"closeLinear";
MPVideoEvent const MPVideoEventCollapse = @"collapse";
MPVideoEvent const MPVideoEventCompanionAdClick = @"companionAdClick";
MPVideoEvent const MPVideoEventCompanionAdView = @"companionAdView";
MPVideoEvent const MPVideoEventComplete = @"complete";
MPVideoEvent const MPVideoEventCreativeView = @"creativeView";
MPVideoEvent const MPVideoEventError = @"error";
MPVideoEvent const MPVideoEventExitFullScreen = @"exitFullscreen";
MPVideoEvent const MPVideoEventExpand = @"expand";
MPVideoEvent const MPVideoEventFirstQuartile = @"firstQuartile";
MPVideoEvent const MPVideoEventFullScreen = @"fullscreen";
MPVideoEvent const MPVideoEventImpression = @"impression";
MPVideoEvent const MPVideoEventIndustryIconClick = @"industryIconClick";
MPVideoEvent const MPVideoEventIndustryIconView = @"industryIconView";
MPVideoEvent const MPVideoEventMidpoint = @"midpoint";
MPVideoEvent const MPVideoEventMute = @"mute";
MPVideoEvent const MPVideoEventPause = @"pause";
MPVideoEvent const MPVideoEventProgress = @"progress";
MPVideoEvent const MPVideoEventResume = @"resume";
MPVideoEvent const MPVideoEventSkip = @"skip";
MPVideoEvent const MPVideoEventStart = @"start";
MPVideoEvent const MPVideoEventThirdQuartile = @"thirdQuartile";
MPVideoEvent const MPVideoEventUnmute = @"unmute";

@implementation MPVideoEvents

+ (NSSet<MPVideoEvent> *)supported {
    static NSSet<MPVideoEvent> *supportedEvents = nil;
    if (supportedEvents == nil) {
        supportedEvents = [NSSet setWithObjects:
        MPVideoEventClick,
        MPVideoEventClose,
        MPVideoEventCloseLinear,
        MPVideoEventCollapse,
        MPVideoEventCompanionAdClick,
        MPVideoEventCompanionAdView,
        MPVideoEventComplete,
        MPVideoEventCreativeView,
        MPVideoEventError,
        MPVideoEventExitFullScreen,
        MPVideoEventExpand,
        MPVideoEventFirstQuartile,
        MPVideoEventFullScreen,
        MPVideoEventImpression,
        MPVideoEventIndustryIconClick,
        MPVideoEventIndustryIconView,
        MPVideoEventMidpoint,
        MPVideoEventMute,
        MPVideoEventPause,
        MPVideoEventProgress,
        MPVideoEventResume,
        MPVideoEventSkip,
        MPVideoEventStart,
        MPVideoEventThirdQuartile,
        MPVideoEventUnmute,
        nil];
    }

    return supportedEvents;
}

+ (BOOL)isSupportedEvent:(NSString *)event {
    return [MPVideoEvents.supported containsObject:event];
}

@end
