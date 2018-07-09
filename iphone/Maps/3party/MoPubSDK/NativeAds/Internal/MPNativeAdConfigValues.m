//
//  MPNativeAdConfigValues.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
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
