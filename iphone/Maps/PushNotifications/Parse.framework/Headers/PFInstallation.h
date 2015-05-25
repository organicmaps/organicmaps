//
//  PFInstallation.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Parse/PFNullability.h>
#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

PF_ASSUME_NONNULL_BEGIN

/*!
 A Parse Framework Installation Object that is a local representation of an
 installation persisted to the Parse cloud. This class is a subclass of a
 <PFObject>, and retains the same functionality of a PFObject, but also extends
 it with installation-specific fields and related immutability and validity
 checks.

 A valid `PFInstallation` can only be instantiated via
 <[PFInstallation currentInstallation]> because the required identifier fields
 are readonly. The <timeZone> and <badge> fields are also readonly properties which
 are automatically updated to match the device's time zone and application badge
 when the `PFInstallation` is saved, thus these fields might not reflect the
 latest device state if the installation has not recently been saved.

 `PFInstallation` objects which have a valid <deviceToken> and are saved to
 the Parse cloud can be used to target push notifications.

 This class is currently for iOS only. There is no `PFInstallation` for Parse
 applications running on OS X, because they cannot receive push notifications.
 */

@interface PFInstallation : PFObject<PFSubclassing>

///--------------------------------------
/// @name Accessing the Current Installation
///--------------------------------------

/*!
 @abstract Gets the currently-running installation from disk and returns an instance of it.

 @discussion If this installation is not stored on disk, returns a `PFInstallation`
 with <deviceType> and <installationId> fields set to those of the
 current installation.

 @result Returns a `PFInstallation` that represents the currently-running installation.
 */
+ (instancetype)currentInstallation;

///--------------------------------------
/// @name Installation Properties
///--------------------------------------

/*!
 @abstract The device type for the `PFInstallation`.
 */
@property (nonatomic, strong, readonly) NSString *deviceType;

/*!
 @abstract The installationId for the `PFInstallation`.
 */
@property (nonatomic, strong, readonly) NSString *installationId;

/*!
 @abstract The device token for the `PFInstallation`.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSString *deviceToken;

/*!
 @abstract The badge for the `PFInstallation`.
 */
@property (nonatomic, assign) NSInteger badge;

/*!
 @abstract The name of the time zone for the `PFInstallation`.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong, readonly) NSString *timeZone;

/*!
 @abstract The channels for the `PFInstallation`.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, strong) NSArray *channels;

/*!
 @abstract Sets the device token string property from an `NSData`-encoded token.

 @param deviceTokenData A token that identifies the device.
 */
- (void)setDeviceTokenFromData:(PF_NULLABLE NSData *)deviceTokenData;

///--------------------------------------
/// @name Querying for Installations
///--------------------------------------

/*!
 @abstract Creates a <PFQuery> for `PFInstallation` objects.

 @discussion Only the following types of queries are allowed for installations:

 - `[query getObjectWithId:<value>]`
 - `[query whereKey:@"installationId" equalTo:<value>]`
 - `[query whereKey:@"installationId" matchesKey:<key in query> inQuery:<query>]`

 You can add additional query conditions, but one of the above must appear as a top-level `AND` clause in the query.
 */
+ (PF_NULLABLE PFQuery *)query;

@end

PF_ASSUME_NONNULL_END
