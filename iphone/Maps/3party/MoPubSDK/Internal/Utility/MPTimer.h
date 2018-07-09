//
//  MPTimer.h
//  MoPub
//
//  Created by Andrew He on 3/8/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

/*
 * MPTimer wraps an NSTimer and adds pause/resume functionality.
 */
@interface MPTimer : NSObject

@property (nonatomic, copy) NSString *runLoopMode;

+ (MPTimer *)timerWithTimeInterval:(NSTimeInterval)seconds
                            target:(id)target
                          selector:(SEL)aSelector
                           repeats:(BOOL)repeats;

- (BOOL)isValid;
- (void)invalidate;
- (BOOL)isScheduled;
- (BOOL)scheduleNow;
- (BOOL)pause;
- (BOOL)resume;
- (NSTimeInterval)initialTimeInterval;

@end
