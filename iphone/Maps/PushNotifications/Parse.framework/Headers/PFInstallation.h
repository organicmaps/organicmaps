/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

PF_TV_UNAVAILABLE_WARNING
PF_WATCH_UNAVAILABLE_WARNING

NS_ASSUME_NONNULL_BEGIN

/**
 A Parse Framework Installation Object that is a local representation of an
 installation persisted to the Parse cloud. This class is a subclass of a
 `PFObject`, and retains the same functionality of a PFObject, but also extends
 it with installation-specific fields and related immutability and validity
 checks.

 A valid `PFInstallation` can only be instantiated via
 `+currentInstallation` because the required identifier fields
 are readonly. The `timeZone` and `badge` fields are also readonly properties which
 are automatically updated to match the device's time zone and application badge
 when the `PFInstallation` is saved, thus these fields might not reflect the
 latest device state if the installation has not recently been saved.

 `PFInstallation` objects which have a valid `deviceToken` and are saved to
 the Parse cloud can be used to target push notifications.
 */

PF_TV_UNAVAILABLE PF_WATCH_UNAVAILABLE @interface PFInstallation : PFObject<PFSubclassing>

///--------------------------------------
/// @name Accessing the Current Installation
///--------------------------------------

/**
 Gets the currently-running installation from disk and returns an instance of it.

 If this installation is not stored on disk, returns a `PFInstallation`
 with `deviceType` and `installationId` fields set to those of the
 current installation.

 @result Returns a `PFInstallation` that represents the currently-running installation.
 */
+ (instancetype)currentInstallation;

///--------------------------------------
/// @name Installation Properties
///--------------------------------------

/**
 The device type for the `PFInstallation`.
 */
@property (nonatomic, copy, readonly) NSString *deviceType;

/**
 The installationId for the `PFInstallation`.
 */
@property (nonatomic, copy, readonly) NSString *installationId;

/**
 The device token for the `PFInstallation`.
 */
@property (nullable, nonatomic, copy) NSString *deviceToken;

/**
 The badge for the `PFInstallation`.
 */
@property (nonatomic, assign) NSInteger badge;

/**
 The name of the time zone for the `PFInstallation`.
 */
@property (nullable, nonatomic, copy, readonly) NSString *timeZone;

/**
 The channels for the `PFInstallation`.
 */
@property (nullable, nonatomic, copy) NSArray PF_GENERIC(NSString *)*channels;

/**
 Sets the device token string property from an `NSData`-encoded token.

 @param deviceTokenData A token that identifies the device.
 */
- (void)setDeviceTokenFromData:(nullable NSData *)deviceTokenData;

///--------------------------------------
/// @name Querying for Installations
///--------------------------------------

/**
 Creates a `PFQuery` for `PFInstallation` objects.

 Only the following types of queries are allowed for installations:

 - `[query getObjectWithId:<value>]`
 - `[query whereKey:@"installationId" equalTo:<value>]`
 - `[query whereKey:@"installationId" matchesKey:<key in query> inQuery:<query>]`

 You can add additional query conditions, but one of the above must appear as a top-level `AND` clause in the query.
 */
+ (nullable PFQuery *)query;

@end

NS_ASSUME_NONNULL_END
