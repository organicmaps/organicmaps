//
//  MPNativeAdConfigValues.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdConfigValues.h"
#import "MPNativeAdConfigValues+Internal.h"

@implementation MPNativeAdConfigValues

- (instancetype)initWithImpressionMinVisiblePixels:(CGFloat)impressionMinVisiblePixels
                       impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
                       impressionMinVisibleSeconds:(NSTimeInterval)impressionMinVisibleSeconds {
    if (self = [super init]) {
        _impressionMinVisiblePixels = impressionMinVisiblePixels;
        _impressionMinVisiblePercent = impressionMinVisiblePercent;
        _impressionMinVisibleSeconds = impressionMinVisibleSeconds;
    }

    return self;
}

- (BOOL)isImpressionMinVisibleSecondsValid {
    return [self isValidTimeInterval:self.impressionMinVisibleSeconds];
}

- (BOOL)isImpressionMinVisiblePercentValid {
    return [self isValidPercentage:self.impressionMinVisiblePercent];
}

- (BOOL)isImpressionMinVisiblePixelsValid {
    return [self isValidNumberOfPixels:self.impressionMinVisiblePixels];
}

@end
