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

#import "FIRCLSFABAsyncOperation_Private.h"

@interface FIRCLSFABAsyncOperation () {
  BOOL _internalExecuting;
  BOOL _internalFinished;
}

@property(nonatomic, strong) NSRecursiveLock *lock;

@end

@implementation FIRCLSFABAsyncOperation

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _internalExecuting = NO;
  _internalFinished = NO;

  _lock = [[NSRecursiveLock alloc] init];
  _lock.name = [NSString stringWithFormat:@"com.google.firebase.crashlytics.%@-lock", [self class]];
  ;

  return self;
}

#pragma mark - NSOperation Overrides
- (BOOL)isConcurrent {
  return YES;
}

- (BOOL)isAsynchronous {
  return YES;
}

- (BOOL)isExecuting {
  [self.lock lock];
  BOOL result = _internalExecuting;
  [self.lock unlock];

  return result;
}

- (BOOL)isFinished {
  [self.lock lock];
  BOOL result = _internalFinished;
  [self.lock unlock];

  return result;
}

- (void)start {
  if ([self checkForCancellation]) {
    return;
  }

  [self markStarted];

  [self main];
}

#pragma mark - Utilities
- (void)changeValueForKey:(NSString *)key inBlock:(void (^)(void))block {
  [self willChangeValueForKey:key];
  block();
  [self didChangeValueForKey:key];
}

- (void)lock:(void (^)(void))block {
  [self.lock lock];
  block();
  [self.lock unlock];
}

- (BOOL)checkForCancellation {
  if ([self isCancelled]) {
    [self markDone];
    return YES;
  }

  return NO;
}

#pragma mark - State Management
- (void)unlockedMarkFinished {
  [self changeValueForKey:@"isFinished"
                  inBlock:^{
                    self->_internalFinished = YES;
                  }];
}

- (void)unlockedMarkStarted {
  [self changeValueForKey:@"isExecuting"
                  inBlock:^{
                    self->_internalExecuting = YES;
                  }];
}

- (void)unlockedMarkComplete {
  [self changeValueForKey:@"isExecuting"
                  inBlock:^{
                    self->_internalExecuting = NO;
                  }];
}

- (void)markStarted {
  [self lock:^{
    [self unlockedMarkStarted];
  }];
}

- (void)markDone {
  [self lock:^{
    [self unlockedMarkComplete];
    [self unlockedMarkFinished];
  }];
}

#pragma mark - Protected
- (void)finishWithError:(NSError *)error {
  if (self.asyncCompletion) {
    self.asyncCompletion(error);
  }
  [self markDone];
}

@end
