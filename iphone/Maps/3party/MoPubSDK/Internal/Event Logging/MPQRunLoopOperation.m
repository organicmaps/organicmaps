//
//  MPQRunLoopOperation.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

/*
    File:       QRunLoopOperation.m
    Contains:   An abstract subclass of NSOperation for async run loop based operations.
    Written by: DTS
    Copyright:  Copyright (c) 2010 Apple Inc. All Rights Reserved.
    Disclaimer: IMPORTANT: This Apple software is supplied to you by Apple Inc.
                ("Apple") in consideration of your agreement to the following
                terms, and your use, installation, modification or
                redistribution of this Apple software constitutes acceptance of
                these terms.  If you do not agree with these terms, please do
                not use, install, modify or redistribute this Apple software.
                In consideration of your agreement to abide by the following
                terms, and subject to these terms, Apple grants you a personal,
                non-exclusive license, under Apple's copyrights in this
                original Apple software (the "Apple Software"), to use,
                reproduce, modify and redistribute the Apple Software, with or
                without modifications, in source and/or binary forms; provided
                that if you redistribute the Apple Software in its entirety and
                without modifications, you must retain this notice and the
                following text and disclaimers in all such redistributions of
                the Apple Software. Neither the name, trademarks, service marks
                or logos of Apple Inc. may be used to endorse or promote
                products derived from the Apple Software without specific prior
                written permission from Apple.  Except as expressly stated in
                this notice, no other rights or licenses, express or implied,
                are granted by Apple herein, including but not limited to any
                patent rights that may be infringed by your derivative works or
                by other works in which the Apple Software may be incorporated.
                The Apple Software is provided by Apple on an "AS IS" basis.
                APPLE MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
                WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
                MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING
                THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
                COMBINATION WITH YOUR PRODUCTS.
                IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT,
                INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
                TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
                DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY
                OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
                OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY
                OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
                OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF
                SUCH DAMAGE.
*/

#import "MPQRunLoopOperation.h"

@interface MPQRunLoopOperation ()

@property (assign, readwrite) MPQRunLoopOperationState state;
@property (copy, readwrite) NSError *error;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPQRunLoopOperation

// Necessary since auto-synthesize doesn't happen when we manually implement both getter / setter.
@synthesize state = _state;

- (instancetype)init
{
    self = [super init];
    if (self) {
        NSAssert(_state == MPQRunLoopOperationStateInited, @"MPQRunLoopOperation must be in the inited state upon initialization.");
    }
    return self;
}

#pragma mark - Properties

// Returns the effective run loop thread, that is, the one set by the user
// or, if that's not set, the main thread.
- (NSThread *)actualRunLoopThread
{
    return self.runLoopThread ?: [NSThread mainThread];
}

// Returns YES if the current thread is the actual run loop thread.
- (BOOL)isActualRunLoopThread
{
    return [[NSThread currentThread] isEqual:[self actualRunLoopThread]];
}

// Returns the run loop modes in which this operation can be executed. If the user provides a set
// of run loop modes, this method will return that set; otherwise, it will return a set containing
// the default run loop mode.
- (NSSet *)actualRunLoopModes
{
    return ([self.runLoopModes count] != 0) ? self.runLoopModes : [NSSet setWithObject:NSDefaultRunLoopMode];
}

#pragma mark - Core state transitions

- (MPQRunLoopOperationState)state
{
    return _state;
}

- (void)setState:(MPQRunLoopOperationState)newState
{
    @synchronized(self) {
        MPQRunLoopOperationState oldState;
        
        // The following check is really important.  The state can only go forward, and there
        // should be no redundant changes to the state (that is, newState must never be
        // equal to _state).
        
        assert(newState > _state);
        
        // inited    -> executing = update isExecuting
        // inited    -> finished  = update isFinished
        // executing -> finished  = update both isExecuting and isFinished
        
        oldState = _state;
        
        if ((newState == MPQRunLoopOperationStateExecuting) || (oldState == MPQRunLoopOperationStateExecuting)) {
            [self willChangeValueForKey:@"isExecuting"];
        }
        if (newState == MPQRunLoopOperationStateFinished) {
            [self willChangeValueForKey:@"isFinished"];
        }
        
        _state = newState;
        
        if (newState == MPQRunLoopOperationStateFinished) {
            [self didChangeValueForKey:@"isFinished"];
        }
        if ((newState == MPQRunLoopOperationStateExecuting) || (oldState == MPQRunLoopOperationStateExecuting)) {
            [self didChangeValueForKey:@"isExecuting"];
        }
    }
}

- (void)startOnRunLoopThread
{
    NSAssert([self isActualRunLoopThread], @"-startOnRunLoopThread must be called on the run loop thread.");
    NSAssert(self.state == MPQRunLoopOperationStateExecuting, @"The operation should be in the executing state.");
    
    if (self.isCancelled) {
        // We were cancelled before we even got running. Flip the finished state immediately.
        [self finishWithError:[NSError errorWithDomain:NSCocoaErrorDomain code:NSUserCancelledError userInfo:nil]];
    } else {
        [self operationDidStart];
    }
}

- (void)cancelOnRunLoopThread
{
    NSAssert([self isActualRunLoopThread], @"-cancelOnRunLoopThread must be called on the run loop thread.");
    
    // We know that a) state was kQRunLoopOperationStateExecuting when we were
    // scheduled (that's enforced by -cancel), and b) the state can't go
    // backwards (that's enforced by -setState), so we know the state must
    // either be kQRunLoopOperationStateExecuting or kQRunLoopOperationStateFinished.
    // We also know that the transition from executing to finished always
    // happens on the run loop thread.  Thus, we don't need to lock here.
    // We can look at state and, if we're executing, trigger a cancellation.
    
    if (self.state == MPQRunLoopOperationStateExecuting) {
        [self finishWithError:[NSError errorWithDomain:NSCocoaErrorDomain code:NSUserCancelledError userInfo:nil]];
    }
}

- (void)finishWithError:(NSError *)error
{
    NSAssert([self isActualRunLoopThread], @"-finishWithError: must be called on the run loop thread.");
    
    // `error` may be nil, since this method serves as the "exit" point for the operation and will
    // get called in both the success and the failure cases.
    
    if (!self.error) {
        self.error = error;
    }
    
    [self operationWillFinish];
    
    self.state = MPQRunLoopOperationStateFinished;
}

#pragma mark - Subclass override points

- (void)operationDidStart
{
    NSAssert([self isActualRunLoopThread], @"-operationDidStart must be called on the run loop thread.");
}

- (void)operationWillFinish
{
    NSAssert([self isActualRunLoopThread], @"-operationWillFinish must be called on the run loop thread.");
}

#pragma mark - NSOperation overrides

/*
 * All of the following overridden methods must be thread-safe.
 */

- (BOOL)isConcurrent
{
    return YES;
}

- (BOOL)isExecuting
{
    return self.state == MPQRunLoopOperationStateExecuting;
}

- (BOOL)isFinished
{
    return self.state == MPQRunLoopOperationStateFinished;
}

- (void)start
{
    NSAssert(self.state == MPQRunLoopOperationStateInited, @"-start cannot be called on an executing or finished operation.");
    
    // We have to change the state here, otherwise isExecuting won't necessarily return
    // true by the time we return from -start.  Also, we don't test for cancellation
    // here because that would a) result in us sending isFinished notifications on a
    // thread that isn't our run loop thread, and b) confuse the core cancellation code,
    // which expects to run on our run loop thread.  Finally, we don't have to worry
    // about races with other threads calling -start.  Only one thread is allowed to
    // start us at a time
    
    self.state = MPQRunLoopOperationStateExecuting;
    [self performSelector:@selector(startOnRunLoopThread) onThread:self.actualRunLoopThread withObject:nil waitUntilDone:NO modes:[self.actualRunLoopModes allObjects]];
}

- (void)cancel
{
    BOOL shouldRunCancelOnRunLoopThread;
    BOOL wasAlreadyCancelled;
    
    // We need to synchronise here to avoid state changes to isCancelled and state
    // while we're running.
    
    @synchronized(self) {
        wasAlreadyCancelled = self.isCancelled;
        
        // Call our super class so that isCancelled starts returning true immediately.
        [super cancel];
        
        // If we were the one to set isCancelled (that is, we won the race with regards
        // other threads calling -cancel) and we're actually running (that is, we lost
        // the race with other threads calling -start and the run loop thread finishing),
        // we schedule to run on the run loop thread.
        
        shouldRunCancelOnRunLoopThread = !wasAlreadyCancelled && self.state == MPQRunLoopOperationStateExecuting;
    }
    
    if (shouldRunCancelOnRunLoopThread) {
        [self performSelector:@selector(cancelOnRunLoopThread) onThread:[self actualRunLoopThread] withObject:nil waitUntilDone:NO modes:[self.actualRunLoopModes allObjects]];
    }
}

@end
