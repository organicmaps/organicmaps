//
//  MOPUBNativeVideoAdConfigValues.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPNativeAdConfigValues.h"

@interface MOPUBNativeVideoAdConfigValues : MPNativeAdConfigValues

@property (nonatomic, readonly) NSInteger playVisiblePercent;
@property (nonatomic, readonly) NSInteger pauseVisiblePercent;
@property (nonatomic, readonly) NSTimeInterval maxBufferingTime;
@property (nonatomic, readonly) NSDictionary *trackers;

- (instancetype)initWithPlayVisiblePercent:(NSInteger)playVisiblePercent
                       pauseVisiblePercent:(NSInteger)pauseVisiblePercent
                impressionMinVisiblePixels:(CGFloat)impressionMinVisiblePixels
               impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
               impressionMinVisibleSeconds:(NSTimeInterval)impressionMinVisibleSeconds
                          maxBufferingTime:(NSTimeInterval)maxBufferingTime
                                  trackers:(NSDictionary *)trackers NS_DESIGNATED_INITIALIZER;

@property (nonatomic, readonly) BOOL isValid;

- (instancetype)initWithImpressionMinVisiblePixels:(CGFloat)impressionMinVisiblePixels
                       impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
                       impressionMinVisibleSeconds:(NSTimeInterval)impressionMinVisibleSeconds __attribute__((unavailable("initWithImpressionMinVisiblePixels:impressionMinVisiblePercent:impressionMinVisibleSeconds: not available")));

@end
