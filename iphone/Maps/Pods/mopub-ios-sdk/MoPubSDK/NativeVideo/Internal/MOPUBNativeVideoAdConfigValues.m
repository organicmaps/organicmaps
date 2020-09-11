//
//  MOPUBNativeVideoAdConfigValues.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBNativeVideoAdConfigValues.h"
#import "MPNativeAdConfigValues+Internal.h"

@implementation MOPUBNativeVideoAdConfigValues

- (instancetype)initWithPlayVisiblePercent:(NSInteger)playVisiblePercent
                       pauseVisiblePercent:(NSInteger)pauseVisiblePercent
                impressionMinVisiblePixels:(CGFloat)impressionMinVisiblePixels
               impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
               impressionMinVisibleSeconds:(NSTimeInterval)impressionMinVisibleSeconds
                          maxBufferingTime:(NSTimeInterval)maxBufferingTime
                                  trackers:(NSDictionary *)trackers
{
    self = [super initWithImpressionMinVisiblePixels:impressionMinVisiblePixels
                         impressionMinVisiblePercent:impressionMinVisiblePercent
                         impressionMinVisibleSeconds:impressionMinVisibleSeconds];
    if (self) {
        _playVisiblePercent = playVisiblePercent;
        _pauseVisiblePercent = pauseVisiblePercent;
        _maxBufferingTime = maxBufferingTime;
        _trackers = trackers;
    }
    return self;
}

- (BOOL)isValid
{
    return ((self.isImpressionMinVisiblePixelsValid || self.isImpressionMinVisiblePercentValid) && // OR because we only need pixels or percent, not both
            self.isImpressionMinVisibleSecondsValid &&
            [self isValidPercentage:self.playVisiblePercent] &&
            [self isValidPercentage:self.pauseVisiblePercent] &&
            [self isValidTimeInterval:self.maxBufferingTime]);
}

@end
