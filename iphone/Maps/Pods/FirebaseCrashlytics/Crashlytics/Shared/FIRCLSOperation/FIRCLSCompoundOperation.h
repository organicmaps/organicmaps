// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import "FIRCLSFABAsyncOperation.h"

/**
 * If the compound operation is sent a @c -[cancel] message while executing, it will attempt to
 * cancel all operations on its internal queue, and will return an error in its @c asyncCompletion
 * block with this value as its code.
 */
FOUNDATION_EXPORT const NSUInteger FIRCLSCompoundOperationErrorCodeCancelled;

/**
 * If one or more of the operations on the @c compoundQueue fail, this operation returns an error
 * in its @c asyncCompletion block with this code, and an array of @c NSErrors keyed on @c
 * FIRCLSCompoundOperationErrorUserInfoKeyUnderlyingErrors in the @c userInfo dictionary.
 */
FOUNDATION_EXPORT const NSUInteger FIRCLSCompoundOperationErrorCodeSuboperationFailed;

/**
 * When all the operations complete, this @c FIRCLSCompoundOperation instance's @c asyncCompletion
 * block is called. If any errors were passed by the suboperations' @c asyncCompletion blocks, they
 * are put in an array which can be accessed in the @c userInfo dictionary in the error parameter
 * for this instance's @c asyncCompletion block.
 */
FOUNDATION_EXPORT NSString *const FIRCLSCompoundOperationErrorUserInfoKeyUnderlyingErrors;

/**
 * An operation that executes a collection of suboperations on an internal private queue. Any
 * instance of @c FIRCLSFABAsyncOperation passed into this instance's @c operations property has the
 * potential to return an @c NSError in its @c asyncCompletion block. This instance's @c
 * asyncCompletion block will put all such errors in an @c NSArray and return an @c NSError whose @c
 * userInfo contains that array keyed by @c FIRCLSCompoundOperationErrorUserInfoKeyUnderlyingErrors.
 */
@interface FIRCLSCompoundOperation : FIRCLSFABAsyncOperation

/**
 * An array of @c NSOperations to execute, which can include instances of @c FIRCLSFABAsyncOperation
 * or
 * @c FIRCLSCompoundOperation. This operation will not be marked as finished until all suboperations
 * are marked as finished.
 */
@property(copy, nonatomic) NSArray<NSOperation *> *operations;

@property(strong, nonatomic, readonly) NSOperationQueue *compoundQueue;

@end
