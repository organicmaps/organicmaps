//
//  MPNativeAdConfigValues+Internal.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPNativeAdConfigValues+Internal.h"

@implementation MPNativeAdConfigValues (Internal)

- (BOOL)isValidPercentage:(NSInteger)percentage
{
    return (percentage >= 0 && percentage <= 100);
}

- (BOOL)isValidTimeInterval:(NSTimeInterval)timeInterval
{
    return timeInterval > 0.0;
}

- (BOOL)isValidNumberOfPixels:(CGFloat)pixels {
    return pixels >= 0.0;
}

@end
