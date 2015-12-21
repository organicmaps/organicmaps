/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFConstants.h>

NS_ASSUME_NONNULL_BEGIN

/**
 Provides a general interface for delegation of third party authentication with `PFUser`s.
 */
@protocol PFUserAuthenticationDelegate <NSObject>

/**
 Called when restoring third party authentication credentials that have been serialized,
 such as session keys, user id, etc.

 @note This method will be executed on a background thread.

 @param authData The auth data for the provider. This value may be `nil` when unlinking an account.

 @return `YES` - if the `authData` was succesfully synchronized,
 or `NO` if user should not longer be associated because of bad `authData`.
 */
- (BOOL)restoreAuthenticationWithAuthData:(nullable NSDictionary PF_GENERIC(NSString *, NSString *)*)authData;

@end

NS_ASSUME_NONNULL_END
