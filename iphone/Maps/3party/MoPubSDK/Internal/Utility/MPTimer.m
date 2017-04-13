//
//  MPTimer.m
//  MoPub
//
//  Created by Andrew He on 3/8/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPTimer.h"
#import "MPLogging.h"
#import "MPInternalUtils.h"

@interface MPTimer ()
@property (nonatomic, assign) NSTimeInterval timeInterval;
@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, copy) NSDate *pauseDate;
@property (nonatomic, assign) BOOL isPaused;
@end

@interface MPTimer ()

@property (nonatomic, weak) id target;
@property (nonatomic, assign) SEL selector;

@end

@implementation MPTimer

@synthesize timeInterval = _timeInterval;
@synthesize timer = _timer;
@synthesize pauseDate = _pauseDate;
@synthesize target = _target;
@synthesize selector = _selector;
@synthesize isPaused = _isPaused;

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats
{
    MPTimer *timer = [[MPTimer alloc] init];
    timer.target = target;
    timer.selector = aSelector;
    timer.timer = [NSTimer timerWithTimeInterval:seconds
                                      target:timer
                                    selector:@selector(timerDidFire)
                                    userInfo:nil
                                     repeats:repeats];
    timer.timeInterval = seconds;
    timer.runLoopMode = NSDefaultRunLoopMode;
    return timer;
}

- (void)dealloc
{
    [self.timer invalidate];
}

- (void)timerDidFire
{
    SUPPRESS_PERFORM_SELECTOR_LEAK_WARNING(
        [self.target performSelector:self.selector withObject:nil]
    );
}

- (BOOL)isValid
{
    return [self.timer isValid];
}

- (void)invalidate
{
    self.target = nil;
    self.selector = nil;
    [self.timer invalidate];
    self.timer = nil;
}

- (BOOL)isScheduled
{
    if (!self.timer) {
        return NO;
    }
    CFRunLoopRef runLoopRef = [[NSRunLoop currentRunLoop] getCFRunLoop];
    CFArrayRef arrayRef = CFRunLoopCopyAllModes(runLoopRef);
    CFIndex count = CFArrayGetCount(arrayRef);

    for (CFIndex i = 0; i < count; ++i) {
        CFStringRef runLoopMode = CFArrayGetValueAtIndex(arrayRef, i);
        if (CFRunLoopContainsTimer(runLoopRef, (__bridge CFRunLoopTimerRef)self.timer, runLoopMode)) {
            CFRelease(arrayRef);
            return YES;
        }
    }

    CFRelease(arrayRef);
    return NO;
}

- (BOOL)scheduleNow
{
    if (![self.timer isValid]) {
        MPLogDebug(@"Could not schedule invalidated MPTimer (%p).", self);
        return NO;
    }

    [[NSRunLoop currentRunLoop] addTimer:self.timer forMode:self.runLoopMode];
    return YES;
}

- (BOOL)pause
{
    NSTimeInterval secondsLeft;
    if (self.isPaused) {
        MPLogDebug(@"No-op: tried to pause an MPTimer (%p) that was already paused.", self);
        return NO;
    }

    if (![self.timer isValid]) {
        MPLogDebug(@"Cannot pause invalidated MPTimer (%p).", self);
        return NO;
    }

    if (![self isScheduled]) {
        MPLogDebug(@"No-op: tried to pause an MPTimer (%p) that was never scheduled.", self);
        return NO;
    }

    NSDate *fireDate = [self.timer fireDate];
    self.pauseDate = [NSDate date];
    secondsLeft = [fireDate timeIntervalSinceDate:self.pauseDate];
    if (secondsLeft <= 0) {
        MPLogWarn(@"An MPTimer was somehow paused after it was supposed to fire.");
    } else {
        MPLogDebug(@"Paused MPTimer (%p) %.1f seconds left before firing.", self, secondsLeft);
    }

    // Pause the timer by setting its fire date far into the future.
    [self.timer setFireDate:[NSDate distantFuture]];
    self.isPaused = YES;

    return YES;
}

- (BOOL)resume
{
    if (![self.timer isValid]) {
        MPLogDebug(@"Cannot resume invalidated MPTimer (%p).", self);
        return NO;
    }

    if (!self.isPaused) {
        MPLogDebug(@"No-op: tried to resume an MPTimer (%p) that was never paused.", self);
        return NO;
    }

    MPLogDebug(@"Resumed MPTimer (%p), should fire in %.1f seconds.", self.timeInterval);

    // Resume the timer.
    NSDate *newFireDate = [NSDate dateWithTimeInterval:self.timeInterval sinceDate:[NSDate date]];
    [self.timer setFireDate:newFireDate];

    if (![self isScheduled]) {
        [[NSRunLoop currentRunLoop] addTimer:self.timer forMode:self.runLoopMode];
    }

    self.isPaused = NO;
    return YES;
}

- (NSTimeInterval)initialTimeInterval
{
    return self.timeInterval;
}

@end

