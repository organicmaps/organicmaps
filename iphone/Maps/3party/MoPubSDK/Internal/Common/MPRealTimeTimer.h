//
//  MPRealTimeTimer.h
//  MoPubSampleApp
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

/***
 * MPRealTimeTimer is a class meant for situations in which one may want to disaptch an event for later without
 * any regard for application state. Backgrounding and suspension will not affect time keeping. MPRealTimeTimer will NOT
 * fire while the application is backgrounded or suspended, but will fire immediately upon foregrounding if the
 * application is not foregrounded when the time interval elapses.
 *
 * Note: MPRealTimeTimer uses NSTimer as a base and as such perfect accuracy is not guaranteed.
 ***/

@interface MPRealTimeTimer : NSObject

// Initializer which takes in a time interval (from when `scheduleNow` is called) and a block to execute when firing.
- (instancetype)initWithInterval:(NSTimeInterval)interval
                           block:(void(^)(MPRealTimeTimer *))block NS_DESIGNATED_INITIALIZER;

// Returns `YES` if the timer is currently keeping time; `NO` if it's waiting to be scheduled.
@property (nonatomic, assign, readonly) BOOL isScheduled;

// Schedules the MPRealTimeTimer instance to fire at `interval` seconds from now. Calling `scheduleNow` while
// `isScheduled` is set to `YES` will do nothing.
- (void)scheduleNow;

// Executes `block` and stops all time keeping.
- (void)fire;

// Stops all time keeping without executing anything.
- (void)invalidate;

// Use of `init` is not supported.
- (instancetype)init NS_UNAVAILABLE;

@end
