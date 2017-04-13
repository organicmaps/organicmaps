//
//  MPCountdownTimerView.h
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

/**
 * A view that will display a countdown timer and invoke a completion block once
 * the timer has elapsed.
 */
@interface MPCountdownTimerView : UIView

/**
 * Flag indicating if the timer is active.
 */
@property (nonatomic, readonly) BOOL isActive;

/**
 * Flag indicating if the timer is currently paused.
 */
@property (nonatomic, readonly) BOOL isPaused;

/**
 * Initializes a countdown timer view. The timer is not automatically started.
 *
 * @param frame Frame of the view.
 * @param seconds Duration of the timer in seconds. This value must be greater than zero.
 * @returns An initialized timer if successful; otherwise nil.
 */
- (instancetype)initWithFrame:(CGRect)frame duration:(NSTimeInterval)seconds;

/**
 * Starts the countdown timer. If the timer has already started, calling this method again will do nothing.
 *
 * @param completion Completion block that is fired when the timer elapses or is stopped.
 */
- (void)startWithTimerCompletion:(void(^)(BOOL hasElapsed))completion;

/**
 * Stops the timer and optionally invokes the completion block from `startWithTimerCompletion:`.
 * If the timer hasn't started, calling this method will do nothing.
 */
- (void)stopAndSignalCompletion:(BOOL)shouldSignalCompletion;

/**
 * Pauses the timer. If the timer hasn't started, calling this method will do nothing.
 */
- (void)pause;

/**
 * Resumes the timer. If the timer isn't paused, calling this method will do nothing.
 */
- (void)resume;

@end
