//
//  MPNativeAdConfigValues+Internal.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPNativeAdConfigValues.h"

@interface MPNativeAdConfigValues (Internal)

- (BOOL)isValidPercentage:(NSInteger)percentage;
- (BOOL)isValidTimeInterval:(NSTimeInterval)timeInterval;
- (BOOL)isValidNumberOfPixels:(CGFloat)pixels;

@end
