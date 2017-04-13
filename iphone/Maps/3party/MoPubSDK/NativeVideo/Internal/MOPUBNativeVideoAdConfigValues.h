//
//  MOPUBNativeVideoAdConfigValues.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MOPUBNativeVideoAdConfigValues : NSObject

@property (nonatomic, readonly) NSInteger playVisiblePercent;
@property (nonatomic, readonly) NSInteger pauseVisiblePercent;
@property (nonatomic, readonly) NSInteger impressionMinVisiblePercent;
@property (nonatomic, readonly) NSTimeInterval impressionVisible;
@property (nonatomic, readonly) NSTimeInterval maxBufferingTime;
@property (nonatomic, readonly) NSDictionary *trackers;

- (instancetype)initWithPlayVisiblePercent:(NSInteger)playVisiblePercent
                       pauseVisiblePercent:(NSInteger)pauseVisiblePercent
               impressionMinVisiblePercent:(NSInteger)impressionMinVisiblePercent
                         impressionVisible:(NSTimeInterval)impressionVisible
                          maxBufferingTime:(NSTimeInterval)maxBufferingTime trackers:(NSDictionary *)trackers;
- (BOOL)isValid;

@end
