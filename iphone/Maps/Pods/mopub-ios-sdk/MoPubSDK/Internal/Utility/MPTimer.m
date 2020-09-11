//
//  MPTimer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <objc/message.h> // for `objc_msgSend`
#import "MPTimer.h"
#import "MPLogging.h"

@interface MPTimer ()

@property (nonatomic, assign) NSTimeInterval timeInterval;
@property (nonatomic, strong) NSTimer *timer; // lazily created in `scheduleNow`
@property (nonatomic, assign) BOOL isRepeatingTimer;
@property (nonatomic, assign) BOOL isCountdownActive;

@property (nonatomic, weak) id target;
@property (nonatomic, assign) SEL selector;

@end

@implementation MPTimer

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats
                       runLoopMode:(NSString *)runLoopMode
{
    MPTimer *timer = [[MPTimer alloc] init];
    timer.target = target;
    timer.selector = aSelector;
    timer.isCountdownActive = NO;
    timer.isRepeatingTimer = repeats;
    timer.timeInterval = seconds;

    // Initialize the internal `NSTimer`, but set its fire date in the far future.
    // `scheduleNow` will handle the firing of the timer.
    timer.timer = [NSTimer timerWithTimeInterval:seconds
                                          target:timer
                                        selector:@selector(timerDidFire)
                                        userInfo:nil
                                         repeats:repeats];
    [timer.timer setFireDate:NSDate.distantFuture];

    // Runloop scheduling must be performed on the main thread. To prevent
    // a potential deadlock, scheduling to the main thread will be asynchronous
    // on the next main thread run loop.
    void (^mainThreadOperation)(void) = ^void(void) {
        [NSRunLoop.mainRunLoop addTimer:timer.timer forMode:runLoopMode];
    };

    if ([NSThread isMainThread]) {
        mainThreadOperation();
    } else {
        dispatch_async(dispatch_get_main_queue(), mainThreadOperation);
    }

    return timer;
}

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats {
    return [self timerWithTimeInterval:seconds
                                target:target
                              selector:aSelector
                               repeats:repeats
                           runLoopMode:NSDefaultRunLoopMode];
}

- (void)dealloc
{
    [self.timer invalidate];
}

- (BOOL)isValid
{
    return [self.timer isValid];
}

- (void)invalidate
{
    @synchronized (self) {
        self.target = nil;
        self.selector = nil;
        [self.timer invalidate];
        self.timer = nil;
        self.isCountdownActive = NO;
    }
}

- (void)scheduleNow
{
    /*
     Note: `MPLog` statements are commented out because during SDK init, the chain of calls
     `MPConsentManager.sharedManager` -> `newNextUpdateTimer` -> `MPTimer.scheduleNow` ->
     `MPLogDebug` -> `MPIdentityProvider.identifier` -> `MPConsentManager.sharedManager` will cause
     a crash with EXC_BAD_INSTRUCTION: the same `dispatch_once` is called twice for
     `MPConsentManager.sharedManager` in the same call stack. Uncomment the logs after
     `MPIdentityProvider` is refactored.
     */
    @synchronized (self) {
        if (![self.timer isValid]) {
//                MPLogDebug(@"Could not schedule invalidated MPTimer (%p).", self);
            return;
        }

        if (self.isCountdownActive) {
//                MPLogDebug(@"Tried to schedule an MPTimer (%p) that is already ticking.",self);
            return;
        }

        NSDate *newFireDate = [NSDate dateWithTimeInterval:self.timeInterval sinceDate:[NSDate date]];
        [self.timer setFireDate:newFireDate];
        self.isCountdownActive = YES;
    }
}

- (void)pause
{
    @synchronized (self) {
        if (!self.isCountdownActive) {
            MPLogDebug(@"No-op: tried to pause an MPTimer (%p) that was already paused.", self);
            return;
        }

        if (![self.timer isValid]) {
            MPLogDebug(@"Cannot pause invalidated MPTimer (%p).", self);
            return;
        }

        // `fireDate` is the date which the timer will fire. If the timer is no longer valid, `fireDate`
        // is the last date at which the timer fired.
        NSTimeInterval secondsLeft = [[self.timer fireDate] timeIntervalSinceDate:[NSDate date]];
        if (secondsLeft <= 0) {
            MPLogInfo(@"An MPTimer was somehow paused after it was supposed to fire.");
        } else {
            MPLogDebug(@"Paused MPTimer (%p) %.1f seconds left before firing.", self, secondsLeft);
        }

        // Pause the timer by setting its fire date far into the future.
        [self.timer setFireDate:[NSDate distantFuture]];
        self.isCountdownActive = NO;
    }
}

- (void)resume
{
    [self scheduleNow];
}

#pragma mark - Private

- (void)timerDidFire
{
    @synchronized (self) {
        if (!self.isRepeatingTimer) {
            self.isCountdownActive = NO; // this is the last firing
        }

        if (self.selector == nil) {
            MPLogDebug(@"%s `selector` is unexpectedly nil. Return early to avoid crash.", __FUNCTION__);
            return;
        }

        // use `objc_msgSend` to avoid the potential memory leak issue of `performSelector:`
        typedef void (*Message)(id, SEL, id);
        Message func = (Message)objc_msgSend;
        func(self.target, self.selector, self);
    }
}

@end
