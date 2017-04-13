//
//  MOPUBNativeVideoAdConfigValues.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBNativeVideoAdConfigValues.h"

@implementation MOPUBNativeVideoAdConfigValues

- (instancetype)initWithPlayVisiblePercent:(NSInteger)playVisiblePercent
                       pauseVisiblePercent:(NSInteger)pauseVisiblePercent
               impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
                         impressionVisible:(NSTimeInterval)impressionVisible
                                 maxBufferingTime:(NSTimeInterval)maxBufferingTime trackers:(NSDictionary *)trackers
{
    self = [super init];
    if (self) {
        _playVisiblePercent = playVisiblePercent;
        _pauseVisiblePercent = pauseVisiblePercent;
        _impressionMinVisiblePercent = impressionMinVisiblePercent;
        _impressionVisible = impressionVisible;
        _maxBufferingTime = maxBufferingTime;
        _trackers = trackers;
    }
    return self;
}

- (BOOL)isValid
{
    return ([self isValidPercentage:self.playVisiblePercent] &&
            [self isValidPercentage:self.pauseVisiblePercent] &&
            [self isValidPercentage:self.impressionMinVisiblePercent] &&
            [self isValidTimeInterval:self.impressionVisible] &&
            [self isValidTimeInterval:self.maxBufferingTime]);
}

- (BOOL)isValidPercentage:(NSInteger)percentage
{
    return (percentage >= 0 && percentage <= 100);
}

- (BOOL)isValidTimeInterval:(NSTimeInterval)timeInterval
{
    return timeInterval > 0;
}

@end
