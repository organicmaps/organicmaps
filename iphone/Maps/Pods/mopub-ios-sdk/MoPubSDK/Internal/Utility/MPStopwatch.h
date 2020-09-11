//
//  MPStopwatch.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 The @c Stopwatch class keeps track of the delta in foreground time between start and stop signals.
 */
@interface MPStopwatch : NSObject
/**
 Flag indicating that the stopwatch is currently running and tracking foreground duration.
 */
@property (nonatomic, readonly) BOOL isRunning;

/**
 Starts tracking foreground time. If the stopwatch is already running, nothing will happen.
 */
- (void)start;

/**
 Stops tracking foreground time. If the stopwatch has not started yet, the method will return 0.
 */
- (NSTimeInterval)stop;

@end

NS_ASSUME_NONNULL_END
