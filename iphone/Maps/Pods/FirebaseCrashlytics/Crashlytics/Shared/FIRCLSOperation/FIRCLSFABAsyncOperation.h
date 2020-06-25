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

#import <Foundation/Foundation.h>

/**
 *  Completion block that can be called in your subclass implementation. It is up to you when you
 *  want to call it.
 */
typedef void (^FIRCLSFABAsyncOperationCompletionBlock)(NSError *__nullable error);

/**
 *  FIRCLSFABAsyncOperation is a subclass of NSOperation that allows for asynchronous work to be
 *  performed, for things like networking, IPC or UI-driven logic. Create your own subclasses to
 *  encapsulate custom logic.
 *  @warning When subclassing to create your own operations, be sure to call -[finishWithError:] at
 *  some point, or program execution will hang.
 *  @see -[finishWithError:] in FIRCLSFABAsyncOperation_Private.h
 */
@interface FIRCLSFABAsyncOperation : NSOperation

/**
 *  Add a callback method for consumers of your subclasses to set when the asynchronous work is
 *  marked as complete with -[finishWithError:].
 */
@property(copy, nonatomic, nullable) FIRCLSFABAsyncOperationCompletionBlock asyncCompletion;

@end
