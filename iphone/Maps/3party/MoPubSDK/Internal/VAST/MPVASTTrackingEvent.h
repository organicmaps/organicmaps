//
//  MPVASTTrackingEvent.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTDurationOffset;

extern NSString * const MPVASTTrackingEventTypeCreativeView;
extern NSString * const MPVASTTrackingEventTypeStart;
extern NSString * const MPVASTTrackingEventTypeFirstQuartile;
extern NSString * const MPVASTTrackingEventTypeMidpoint;
extern NSString * const MPVASTTrackingEventTypeThirdQuartile;
extern NSString * const MPVASTTrackingEventTypeComplete;
extern NSString * const MPVASTTrackingEventTypeMute;
extern NSString * const MPVASTTrackingEventTypeUnmute;
extern NSString * const MPVASTTrackingEventTypePause;
extern NSString * const MPVASTTrackingEventTypeRewind;
extern NSString * const MPVASTTrackingEventTypeResume;
extern NSString * const MPVASTTrackingEventTypeFullscreen;
extern NSString * const MPVASTTrackingEventTypeExitFullscreen;
extern NSString * const MPVASTTrackingEventTypeExpand;
extern NSString * const MPVASTTrackingEventTypeCollapse;
extern NSString * const MPVASTTrackingEventTypeAcceptInvitationLinear;
extern NSString * const MPVASTTrackingEventTypeCloseLinear;
extern NSString * const MPVASTTrackingEventTypeSkip;
extern NSString * const MPVASTTrackingEventTypeProgress;

@interface MPVASTTrackingEvent : MPVASTModel

@property (nonatomic, copy, readonly) NSString *eventType;
@property (nonatomic, copy, readonly) NSURL *URL;
@property (nonatomic, readonly) MPVASTDurationOffset *progressOffset;

@end
