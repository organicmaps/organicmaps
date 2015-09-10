/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Bolts/BFTask.h>

#import <Parse/PFNullability.h>

PF_ASSUME_NONNULL_BEGIN

@class PFConfig;

typedef void(^PFConfigResultBlock)(PFConfig *PF_NULLABLE_S config, NSError *PF_NULLABLE_S error);

/*!
 `PFConfig` is a representation of the remote configuration object.
 It enables you to add things like feature gating, a/b testing or simple "Message of the day".
 */
@interface PFConfig : NSObject

///--------------------------------------
/// @name Current Config
///--------------------------------------

/*!
 @abstract Returns the most recently fetched config.

 @discussion If there was no config fetched - this method will return an empty instance of `PFConfig`.

 @returns Current, last fetched instance of PFConfig.
 */
+ (PFConfig *)currentConfig;

///--------------------------------------
/// @name Retrieving Config
///--------------------------------------

/*!
 @abstract Gets the `PFConfig` object *synchronously* from the server.

 @returns Instance of `PFConfig` if the operation succeeded, otherwise `nil`.
 */
+ (PF_NULLABLE PFConfig *)getConfig;

/*!
 @abstract Gets the `PFConfig` object *synchronously* from the server and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Instance of PFConfig if the operation succeeded, otherwise `nil`.
 */
+ (PF_NULLABLE PFConfig *)getConfig:(NSError **)error;

/*!
 @abstract Gets the `PFConfig` *asynchronously* and sets it as a result of a task.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)getConfigInBackground;

/*!
 @abstract Gets the `PFConfig` *asynchronously* and executes the given callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(PFConfig *config, NSError *error)`.
 */
+ (void)getConfigInBackgroundWithBlock:(PF_NULLABLE PFConfigResultBlock)block;

///--------------------------------------
/// @name Parameters
///--------------------------------------

/*!
 @abstract Returns the object associated with a given key.

 @param key The key for which to return the corresponding configuration value.

 @returns The value associated with `key`, or `nil` if there is no such value.
 */
- (PF_NULLABLE_S id)objectForKey:(NSString *)key;

/*!
 @abstract Returns the object associated with a given key.

 @discussion This method enables usage of literal syntax on `PFConfig`.
 E.g. `NSString *value = config[@"key"];`

 @see objectForKey:

 @param keyedSubscript The keyed subscript for which to return the corresponding configuration value.

 @returns The value associated with `key`, or `nil` if there is no such value.
 */
- (PF_NULLABLE_S id)objectForKeyedSubscript:(NSString *)keyedSubscript;

@end

PF_ASSUME_NONNULL_END
