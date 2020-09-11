//
//  MPTimer.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * @c MPTimer is a thread safe @c NSTimer wrapper, with pause / resume functionality.
 */
@interface MPTimer : NSObject

/**
 * Return NO is the timer is paused, and return YES otherwise.
 */
@property (nonatomic, readonly) BOOL isCountdownActive;

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats
                       runLoopMode:(NSString *)runLoopMode;

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats;

- (BOOL)isValid;
- (void)invalidate;
- (void)scheduleNow;
- (void)pause;
- (void)resume;

@end

NS_ASSUME_NONNULL_END
