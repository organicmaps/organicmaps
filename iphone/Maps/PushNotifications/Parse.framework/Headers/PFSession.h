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

#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

NS_ASSUME_NONNULL_BEGIN

@class PFSession;

typedef void(^PFSessionResultBlock)(PFSession *__nullable session, NSError *__nullable error);

/*!
 `PFSession` is a local representation of a session.
 This class is a subclass of a <PFObject>,
 and retains the same functionality as any other subclass of <PFObject>.
 */
@interface PFSession : PFObject<PFSubclassing>

/*!
 @abstract The session token string for this session.
 */
@property (nullable, nonatomic, copy, readonly) NSString *sessionToken;

/*!
 *Asynchronously* fetches a `PFSession` object related to the current user.

 @returns A task that is `completed` with an instance of `PFSession` class or is `faulted` if the operation fails.
 */
+ (BFTask PF_GENERIC(PFSession *)*)getCurrentSessionInBackground;

/*!
 *Asynchronously* fetches a `PFSession` object related to the current user.

 @param block The block to execute when the operation completes.
 It should have the following argument signature: `^(PFSession *session, NSError *error)`.
 */
+ (void)getCurrentSessionInBackgroundWithBlock:(nullable PFSessionResultBlock)block;

@end

NS_ASSUME_NONNULL_END
