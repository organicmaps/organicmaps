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

#import <Foundation/Foundation.h>

@class FBLPromise<ValueType>;

NS_ASSUME_NONNULL_BEGIN

/**
 * The class makes sure the a single operation (represented by a promise) is performed at a time. If
 * there is an ongoing operation, then its existing corresponding promise will be returned instead
 * of starting a new operation.
 */
@interface FIRInstallationsSingleOperationPromiseCache<__covariant ResultType> : NSObject

- (instancetype)init NS_UNAVAILABLE;

/**
 * The designated initializer.
 * @param newOperationHandler The block that must return a new promise representing the
 * single-at-a-time operation. The promise should be fulfilled when the operation is completed. The
 * factory block will be used to create a new promise when needed.
 */
- (instancetype)initWithNewOperationHandler:
    (FBLPromise<ResultType> *_Nonnull (^)(void))newOperationHandler NS_DESIGNATED_INITIALIZER;

/**
 * Creates a new promise or returns an existing pending one.
 * @return Returns and existing pending promise if exists. If the pending promise does not exist
 * then a new one will be created using the `factory` block passed in the initializer. Once the
 * pending promise gets resolved, it is removed, so calling the method again will lead to creating
 * and caching another promise.
 */
- (FBLPromise<ResultType> *)getExistingPendingOrCreateNewPromise;

/**
 * Returns an existing pending promise or `nil`.
 * @return Returns an existing pending promise if there is one or `nil` otherwise.
 */
- (nullable FBLPromise<ResultType> *)getExistingPendingPromise;

@end

NS_ASSUME_NONNULL_END
