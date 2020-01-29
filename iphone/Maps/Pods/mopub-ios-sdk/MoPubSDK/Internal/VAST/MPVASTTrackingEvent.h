//
//  MPVASTTrackingEvent.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

// To learn more about these events, please see VAST documentation:
//      https://www.iab.com/wp-content/uploads/2015/06/VASTv3_0.pdf
typedef NSString * MPVideoEvent;

// keep this list sorted alphabetically
extern MPVideoEvent const MPVideoEventClick;
extern MPVideoEvent const MPVideoEventCloseLinear;
extern MPVideoEvent const MPVideoEventCollapse;
extern MPVideoEvent const MPVideoEventComplete;
extern MPVideoEvent const MPVideoEventCreativeView;
extern MPVideoEvent const MPVideoEventError;
extern MPVideoEvent const MPVideoEventExitFullScreen;
extern MPVideoEvent const MPVideoEventExpand;
extern MPVideoEvent const MPVideoEventFirstQuartile;
extern MPVideoEvent const MPVideoEventFullScreen;
extern MPVideoEvent const MPVideoEventImpression;
extern MPVideoEvent const MPVideoEventMidpoint;
extern MPVideoEvent const MPVideoEventMute;
extern MPVideoEvent const MPVideoEventPause;
extern MPVideoEvent const MPVideoEventProgress;
extern MPVideoEvent const MPVideoEventResume;
extern MPVideoEvent const MPVideoEventSkip;
extern MPVideoEvent const MPVideoEventStart;
extern MPVideoEvent const MPVideoEventThirdQuartile;
extern MPVideoEvent const MPVideoEventUnmute;

@class MPVASTDurationOffset;

@interface MPVASTTrackingEvent : MPVASTModel

@property (nonatomic, copy, readonly) MPVideoEvent eventType;
@property (nonatomic, copy, readonly) NSURL *URL;
@property (nonatomic, readonly) MPVASTDurationOffset *progressOffset;

- (instancetype)initWithEventType:(MPVideoEvent)eventType
                              url:(NSURL *)url
                   progressOffset:(MPVASTDurationOffset *)progressOffset;

@end
