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

#import "FIRCLSCompoundOperation.h"

#import "FIRCLSFABAsyncOperation_Private.h"

#define FIRCLS_DISPATCH_QUEUES_AS_OBJECTS OS_OBJECT_USE_OBJC_RETAIN_RELEASE

const NSUInteger FIRCLSCompoundOperationErrorCodeCancelled = UINT_MAX - 1;
const NSUInteger FIRCLSCompoundOperationErrorCodeSuboperationFailed = UINT_MAX - 2;

NSString *const FIRCLSCompoundOperationErrorUserInfoKeyUnderlyingErrors =
    @"com.google.firebase.crashlytics.FIRCLSCompoundOperation.error.user-info-key.underlying-"
    @"errors";

static NSString *const FIRCLSCompoundOperationErrorDomain =
    @"com.google.firebase.crashlytics.FIRCLSCompoundOperation.error";
static char *const FIRCLSCompoundOperationCountingQueueLabel =
    "com.google.firebase.crashlytics.FIRCLSCompoundOperation.dispatch-queue.counting-queue";

@interface FIRCLSCompoundOperation ()

@property(strong, nonatomic, readwrite) NSOperationQueue *compoundQueue;
@property(assign, nonatomic) NSUInteger completedOperations;
@property(strong, nonatomic) NSMutableArray *errors;
#if FIRCLS_DISPATCH_QUEUES_AS_OBJECTS
@property(strong, nonatomic) dispatch_queue_t countingQueue;
#else
@property(assign, nonatomic) dispatch_queue_t countingQueue;
#endif

@end

@implementation FIRCLSCompoundOperation

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _compoundQueue = [[NSOperationQueue alloc] init];
  _completedOperations = 0;
  _errors = [NSMutableArray array];
  _countingQueue =
      dispatch_queue_create(FIRCLSCompoundOperationCountingQueueLabel, DISPATCH_QUEUE_SERIAL);

  return self;
}

#if !FIRCLS_DISPATCH_QUEUES_AS_OBJECTS
- (void)dealloc {
  if (_countingQueue) {
    dispatch_release(_countingQueue);
  }
}
#endif

- (void)main {
  for (FIRCLSFABAsyncOperation *operation in self.operations) {
    [self injectCompoundAsyncCompletionInOperation:operation];
    [self injectCompoundSyncCompletionInOperation:operation];

    [self.compoundQueue addOperation:operation];
  }
}

- (void)cancel {
  if (self.compoundQueue.operations.count > 0) {
    [self.compoundQueue cancelAllOperations];
    dispatch_sync(self.countingQueue, ^{
      [self attemptCompoundCompletion];
    });
  } else {
    for (NSOperation *operation in self.operations) {
      [operation cancel];
    }

    // we have to add the operations to the queue in order for their isFinished property to be set
    // to true.
    [self.compoundQueue addOperations:self.operations waitUntilFinished:NO];
  }
  [super cancel];
}

- (void)injectCompoundAsyncCompletionInOperation:(FIRCLSFABAsyncOperation *)operation {
  __weak FIRCLSCompoundOperation *weakSelf = self;
  FIRCLSFABAsyncOperationCompletionBlock originalAsyncCompletion = [operation.asyncCompletion copy];
  FIRCLSFABAsyncOperationCompletionBlock completion = ^(NSError *error) {
    __strong FIRCLSCompoundOperation *strongSelf = weakSelf;

    if (originalAsyncCompletion) {
      dispatch_sync(strongSelf.countingQueue, ^{
        originalAsyncCompletion(error);
      });
    }

    [strongSelf updateCompletionCountsWithError:error];
  };
  operation.asyncCompletion = completion;
}

- (void)injectCompoundSyncCompletionInOperation:(FIRCLSFABAsyncOperation *)operation {
  __weak FIRCLSCompoundOperation *weakSelf = self;
  void (^originalSyncCompletion)(void) = [operation.completionBlock copy];
  void (^completion)(void) = ^{
    __strong FIRCLSCompoundOperation *strongSelf = weakSelf;

    if (originalSyncCompletion) {
      dispatch_sync(strongSelf.countingQueue, ^{
        originalSyncCompletion();
      });
    }

    dispatch_sync(strongSelf.countingQueue, ^{
      [strongSelf attemptCompoundCompletion];
    });
  };
  operation.completionBlock = completion;
}

- (void)updateCompletionCountsWithError:(NSError *)error {
  dispatch_sync(self.countingQueue, ^{
    if (!error) {
      self.completedOperations += 1;
    } else {
      [self.errors addObject:error];
    }
  });
}

- (void)attemptCompoundCompletion {
  if (self.isCancelled) {
    [self finishWithError:[NSError errorWithDomain:FIRCLSCompoundOperationErrorDomain
                                              code:FIRCLSCompoundOperationErrorCodeCancelled
                                          userInfo:@{
                                            NSLocalizedDescriptionKey : [NSString
                                                stringWithFormat:@"%@ cancelled", self.name]
                                          }]];
    self.asyncCompletion = nil;
  } else if (self.completedOperations + self.errors.count == self.operations.count) {
    NSError *error = nil;
    if (self.errors.count > 0) {
      error = [NSError
          errorWithDomain:FIRCLSCompoundOperationErrorDomain
                     code:FIRCLSCompoundOperationErrorCodeSuboperationFailed
                 userInfo:@{FIRCLSCompoundOperationErrorUserInfoKeyUnderlyingErrors : self.errors}];
    }
    [self finishWithError:error];
  }
}

@end
