//
//  MPRateLimitConfiguration.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRateLimitConfiguration.h"
#import "MPRealTimeTimer.h"

@interface MPRateLimitConfiguration ()

@property (nonatomic, strong) MPRealTimeTimer * timer;

@end

@implementation MPRateLimitConfiguration

- (BOOL)isRateLimited {
    return self.timer != nil;
}

- (void)setRateLimitTimerWithMilliseconds:(NSInteger)milliseconds reason:(NSString *)reason {
    @synchronized(self) {
        // Intentionally treat accidental less than 0 as 0
        if (milliseconds < 0) {
            milliseconds = 0;
        }

        // If already rate limited, reset the timer by invalidating the present timer. This guarantees the rate limit value from the most recent response is used.
        if (self.isRateLimited) {
            [self.timer invalidate];
        }

        _lastRateLimitMilliseconds = milliseconds;
        _lastRateLimitReason = reason;

        if (milliseconds == 0) {
            self.timer = nil;
            return;
        }

        __weak __typeof__(self) weakSelf = self;
        self.timer = [[MPRealTimeTimer alloc] initWithInterval:(((double)milliseconds) / 1000.0) block:^(MPRealTimeTimer * timer) {
            [weakSelf.timer invalidate];
            weakSelf.timer = nil;
        }];
        [self.timer scheduleNow];
    }
}

@end
