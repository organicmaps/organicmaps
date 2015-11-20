/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#import <Foundation/Foundation.h>

#import <Bolts/BFDefines.h>

NS_ASSUME_NONNULL_BEGIN

@class BFTask BF_GENERIC(BFGenericType);

/*!
 A BFTaskCompletionSource represents the producer side of tasks.
 It is a task that also has methods for changing the state of the
 task by settings its completion values.
 */
@interface BFTaskCompletionSource BF_GENERIC(__covariant BFGenericType) : NSObject

/*!
 Creates a new unfinished task.
 */
+ (instancetype)taskCompletionSource;

/*!
 The task associated with this TaskCompletionSource.
 */
@property (nonatomic, strong, readonly) BFTask BF_GENERIC(BFGenericType) *task;

/*!
 Completes the task by setting the result.
 Attempting to set this for a completed task will raise an exception.
 @param result The result of the task.
 */
- (void)setResult:(nullable BFGenericType)result;

/*!
 Completes the task by setting the error.
 Attempting to set this for a completed task will raise an exception.
 @param error The error for the task.
 */
- (void)setError:(NSError *)error;

/*!
 Completes the task by setting an exception.
 Attempting to set this for a completed task will raise an exception.
 @param exception The exception for the task.
 */
- (void)setException:(NSException *)exception;

/*!
 Completes the task by marking it as cancelled.
 Attempting to set this for a completed task will raise an exception.
 */
- (void)cancel;

/*!
 Sets the result of the task if it wasn't already completed.
 @returns whether the new value was set.
 */
- (BOOL)trySetResult:(nullable BFGenericType)result;

/*!
 Sets the error of the task if it wasn't already completed.
 @param error The error for the task.
 @returns whether the new value was set.
 */
- (BOOL)trySetError:(NSError *)error;

/*!
 Sets the exception of the task if it wasn't already completed.
 @param exception The exception for the task.
 @returns whether the new value was set.
 */
- (BOOL)trySetException:(NSException *)exception;

/*!
 Sets the cancellation state of the task if it wasn't already completed.
 @returns whether the new value was set.
 */
- (BOOL)trySetCancelled;

@end

NS_ASSUME_NONNULL_END
