//
//  MPStopwatch.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPTimer.h"
#import "MPStopwatch.h"
#import "NSDate+MPAdditions.h"

static const NSTimeInterval kStopwatchStep = 0.1; // 100ms interval

@interface MPStopwatch()
@property (nonatomic, assign) NSTimeInterval duration;
@property (nonatomic, strong) MPTimer * timer;
@end

@implementation MPStopwatch

- (instancetype)init {
    if (self = [super init]) {
        _duration = 0.0;
        _timer = nil;
    }

    return self;
}

- (BOOL)isRunning {
    return (self.timer != nil);
}

- (void)start {
    // Stopwatch is running; do nothing.
    if (self.timer != nil) {
        return;
    }

    // Reset internal state and spin up a new timer.
    self.duration = 0.0;
    self.timer = [MPTimer timerWithTimeInterval:kStopwatchStep target:self selector:@selector(onTimerFired) repeats:YES runLoopMode:NSRunLoopCommonModes];

    // Start the countup timer.
    [self.timer scheduleNow];
}

- (NSTimeInterval)stop {
    // Stopwatch not running; return 0.
    if (self.timer == nil) {
        return 0.0;
    }

    // Stop and kill the internal timer.
    [self.timer pause];
    [self.timer invalidate];
    self.timer = nil;

    return self.duration;
}

- (void)onTimerFired {
    self.duration += kStopwatchStep;
}

@end
