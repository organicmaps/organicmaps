//
//  MOPUBNativeVideoImpressionAgent.m
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBNativeVideoImpressionAgent.h"
#import "MPGlobal.h"

static const NSTimeInterval kInvalidTimestamp = -1;

@interface MOPUBNativeVideoImpressionAgent ()

@property (nonatomic) CGFloat requiredVisiblePercentage;
@property (nonatomic) NSTimeInterval requiredPlaybackDuration;
@property (nonatomic) NSTimeInterval visibilitySatisfiedPlaybackTime;
@property (nonatomic, weak) UIView *measuredVideoView;
@property (nonatomic) BOOL requirementsSatisfied;

@end

@implementation MOPUBNativeVideoImpressionAgent

- (instancetype)initWithVideoView:(UIView *)videoView requiredVisibilityPercentage:(CGFloat)visiblePercentage requiredPlaybackDuration:(NSTimeInterval)playbackDuration
{
    self = [super init];

    if (self) {
        _measuredVideoView = videoView;
        _requiredVisiblePercentage = visiblePercentage;
        _requiredPlaybackDuration = playbackDuration;
        _visibilitySatisfiedPlaybackTime = kInvalidTimestamp;
        _requirementsSatisfied = NO;
    }

    return self;
}

- (BOOL)shouldTrackImpressionWithCurrentPlaybackTime:(NSTimeInterval)currentPlaybackTime
{
    // this class's work is done once the requirements are met
    if (!self.requirementsSatisfied) {
        if (MPViewIntersectsParentWindowWithPercent(self.measuredVideoView, self.requiredVisiblePercentage)) {
            // if this is the first time we satisfied the visibility requirement or
            // if the user replays the video and we haven't satisfied the impression yet, set satisfied playback timestamp
            if (self.visibilitySatisfiedPlaybackTime == kInvalidTimestamp || self.visibilitySatisfiedPlaybackTime > currentPlaybackTime) {
                self.visibilitySatisfiedPlaybackTime = currentPlaybackTime;
            }

            // we consider the requirements met if the visibility requirements are met for requiredPlaybackDuration seconds of actual continuous playback
            if (currentPlaybackTime - self.visibilitySatisfiedPlaybackTime >= self.requiredPlaybackDuration) {
                self.requirementsSatisfied = YES;
            }
        } else {
            self.visibilitySatisfiedPlaybackTime = kInvalidTimestamp;
        }
    }

    return self.requirementsSatisfied;
}

@end
