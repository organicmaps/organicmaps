/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "FIRInstallationsSingleOperationPromiseCache.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

@interface FIRInstallationsSingleOperationPromiseCache <ResultType>()
@property(nonatomic, readonly) FBLPromise *_Nonnull (^newOperationHandler)(void);
@property(nonatomic, nullable) FBLPromise *pendingPromise;
@end

@implementation FIRInstallationsSingleOperationPromiseCache

- (instancetype)initWithNewOperationHandler:
    (FBLPromise<id> *_Nonnull (^)(void))newOperationHandler {
  if (newOperationHandler == nil) {
    [NSException raise:NSInvalidArgumentException
                format:@"`newOperationHandler` must not be `nil`."];
  }

  self = [super init];
  if (self) {
    _newOperationHandler = [newOperationHandler copy];
  }
  return self;
}

- (FBLPromise *)getExistingPendingOrCreateNewPromise {
  @synchronized(self) {
    if (!self.pendingPromise) {
      self.pendingPromise = self.newOperationHandler();

      self.pendingPromise
          .then(^id(id result) {
            @synchronized(self) {
              self.pendingPromise = nil;
              return nil;
            }
          })
          .catch(^void(NSError *error) {
            @synchronized(self) {
              self.pendingPromise = nil;
            }
          });
    }

    return self.pendingPromise;
  }
}

- (nullable FBLPromise *)getExistingPendingPromise {
  @synchronized(self) {
    return self.pendingPromise;
  }
}

@end
