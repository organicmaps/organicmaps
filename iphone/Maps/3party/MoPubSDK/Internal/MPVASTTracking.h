//
//  MPVASTTracking.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MPVideoConfig;

typedef NS_ENUM(NSUInteger, MPVideoEventType) {
    MPVideoEventTypeTimeUpdate = 0,
    MPVideoEventTypeMuted,
    MPVideoEventTypeUnmuted,
    MPVideoEventTypePause,
    MPVideoEventTypeResume,
    MPVideoEventTypeFullScreen,
    MPVideoEventTypeExitFullScreen,
    MPVideoEventTypeExpand,
    MPVideoEventTypeCollapse,
    MPVideoEventTypeCompleted,
    MPVideoEventTypeImpression,
    MPVideoEventTypeClick,
    MPVideoEventTypeError
};

@interface MPVASTTracking : NSObject

@property (nonatomic, readonly) MPVideoConfig *videoConfig;
@property (nonatomic) NSTimeInterval videoDuration;

- (instancetype)initWithMPVideoConfig:(MPVideoConfig *)videoConfig videoView:(UIView *)videoView;
- (void)handleVideoEvent:(MPVideoEventType)videoEventType videoTimeOffset:(NSTimeInterval)timeOffset;
- (void)handleNewVideoView:(UIView *)videoView;

@end
