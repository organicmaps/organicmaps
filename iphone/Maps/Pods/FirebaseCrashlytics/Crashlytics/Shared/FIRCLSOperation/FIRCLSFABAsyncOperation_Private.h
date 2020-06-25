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

@interface FIRCLSFABAsyncOperation (Private)

/**
 * Subclasses must call this method when they are done performing work. When it is called is up to
 * you; it can be directly after kicking of a network request, say, or in the callback for its
 * response. Once this method is called, the operation queue it is on will begin executing the next
 * waiting operation. If you directly invoked -[start] on the instance, execution will proceed to
 * the next code statement.
 * @note as soon as this method is called, @c NSOperation's standard @c completionBlock will be
 * executed if one exists, as a result of setting the operation's isFinished property to YES, and
 * the asyncCompletion block is called.
 * @param error Any error to pass to asyncCompletion, or nil if there is none.
 */
- (void)finishWithError:(NSError *__nullable)error;

@end
