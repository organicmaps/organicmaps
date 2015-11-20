/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Parse/PFConstants.h>

PF_TV_UNAVAILABLE_WARNING
PF_WATCH_UNAVAILABLE_WARNING

NS_ASSUME_NONNULL_BEGIN

/*!
 `PFNetworkActivityIndicatorManager` manages the state of the network activity indicator in the status bar.
 When enabled, it will start managing the network activity indicator in the status bar,
 according to the network operations that are performed by Parse SDK.

 The number of active requests is incremented or decremented like a stack or a semaphore,
 the activity indicator will animate, as long as the number is greater than zero.
 */
PF_TV_UNAVAILABLE PF_WATCH_UNAVAILABLE @interface PFNetworkActivityIndicatorManager : NSObject

/*!
 A Boolean value indicating whether the manager is enabled.
 If `YES` - the manager will start managing the status bar network activity indicator,
 according to the network operations that are performed by Parse SDK.
 The default value is `YES`.
 */
@property (nonatomic, assign, getter = isEnabled) BOOL enabled;

/*!
 A Boolean value indicating whether the network activity indicator is currently displayed in the status bar.
 */
@property (nonatomic, assign, readonly, getter = isNetworkActivityIndicatorVisible) BOOL networkActivityIndicatorVisible;

/*!
 The value that indicates current network activities count.
 */
@property (nonatomic, assign, readonly) NSUInteger networkActivityCount;

/*!
 @abstract Returns the shared network activity indicator manager object for the system.

 @returns The systemwide network activity indicator manager.
 */
+ (PFNetworkActivityIndicatorManager *)sharedManager;

/*!
 @abstract Increments the number of active network requests.

 @discussion If this number was zero before incrementing,
 this will start animating network activity indicator in the status bar.
 */
- (void)incrementActivityCount;

/*!
 @abstract Decrements the number of active network requests.

 @discussion If this number becomes zero after decrementing,
 this will stop animating network activity indicator in the status bar.
 */
- (void)decrementActivityCount;

@end

NS_ASSUME_NONNULL_END
