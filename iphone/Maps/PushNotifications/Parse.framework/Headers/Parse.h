//
//  Parse.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE

#import <Parse/PFACL.h>
#import <Parse/PFAnalytics.h>
#import <Parse/PFAnonymousUtils.h>
#import <Parse/PFCloud.h>
#import <Parse/PFConfig.h>
#import <Parse/PFConstants.h>
#import <Parse/PFFile.h>
#import <Parse/PFGeoPoint.h>
#import <Parse/PFObject+Subclass.h>
#import <Parse/PFObject.h>
#import <Parse/PFQuery.h>
#import <Parse/PFRelation.h>
#import <Parse/PFRole.h>
#import <Parse/PFSession.h>
#import <Parse/PFSubclassing.h>
#import <Parse/PFUser.h>
#import <Parse/PFInstallation.h>
#import <Parse/PFNetworkActivityIndicatorManager.h>
#import <Parse/PFNullability.h>
#import <Parse/PFProduct.h>
#import <Parse/PFPurchase.h>
#import <Parse/PFPush.h>
#import <Parse/PFTwitterUtils.h>

#else

#import <ParseOSX/PFACL.h>
#import <ParseOSX/PFAnalytics.h>
#import <ParseOSX/PFAnonymousUtils.h>
#import <ParseOSX/PFCloud.h>
#import <ParseOSX/PFConfig.h>
#import <ParseOSX/PFConstants.h>
#import <ParseOSX/PFFile.h>
#import <ParseOSX/PFGeoPoint.h>
#import <ParseOSX/PFInstallation.h>
#import <ParseOSX/PFNullability.h>
#import <ParseOSX/PFObject+Subclass.h>
#import <ParseOSX/PFObject.h>
#import <ParseOSX/PFPush.h>
#import <ParseOSX/PFQuery.h>
#import <ParseOSX/PFRelation.h>
#import <ParseOSX/PFRole.h>
#import <ParseOSX/PFSession.h>
#import <ParseOSX/PFSubclassing.h>
#import <ParseOSX/PFUser.h>

#endif

PF_ASSUME_NONNULL_BEGIN

/*!
 The `Parse` class contains static functions that handle global configuration for the Parse framework.
 */
@interface Parse : NSObject

///--------------------------------------
/// @name Connecting to Parse
///--------------------------------------

/*!
 @abstract Sets the applicationId and clientKey of your application.

 @param applicationId The application id of your Parse application.
 @param clientKey The client key of your Parse application.
 */
+ (void)setApplicationId:(NSString *)applicationId clientKey:(NSString *)clientKey;

/*!
 @abstract The current application id that was used to configure Parse framework.
 */
+ (NSString *)getApplicationId;

/*!
 @abstract The current client key that was used to configure Parse framework.
 */
+ (NSString *)getClientKey;

///--------------------------------------
/// @name Enabling Local Datastore
///--------------------------------------

/*!
 @abstract Enable pinning in your application. This must be called before your application can use
 pinning. The recommended way is to call this method before `setApplicationId:clientKey:`.
 */
+ (void)enableLocalDatastore;

/*!
 @abstract Flag that indicates whether Local Datastore is enabled.

 @returns `YES` if Local Datastore is enabled, otherwise `NO`.
 */
+ (BOOL)isLocalDatastoreEnabled;

///--------------------------------------
/// @name Enabling Extensions Data Sharing
///--------------------------------------

/*!
 @abstract Enables data sharing with an application group identifier.

 @discussion After enabling - Local Datastore, `currentUser`, `currentInstallation` and all eventually commands
 are going to be available to every application/extension in a group that have the same Parse applicationId.

 @warning This method is required to be called before <setApplicationId:clientKey:>.

 @param groupIdentifier Application Group Identifier to share data with.
 */
+ (void)enableDataSharingWithApplicationGroupIdentifier:(NSString *)groupIdentifier PF_EXTENSION_UNAVAILABLE("Use `enableDataSharingWithApplicationGroupIdentifier:containingApplication:`.");

/*!
 @abstract Enables data sharing with an application group identifier.

 @discussion After enabling - Local Datastore, `currentUser`, `currentInstallation` and all eventually commands
 are going to be available to every application/extension in a group that have the same Parse applicationId.

 @warning This method is required to be called before <setApplicationId:clientKey:>.
 This method can only be used by application extensions.

 @param groupIdentifier Application Group Identifier to share data with.
 @param bundleIdentifier Bundle identifier of the containing application.
 */
+ (void)enableDataSharingWithApplicationGroupIdentifier:(NSString *)groupIdentifier
                                  containingApplication:(NSString *)bundleIdentifier;

/*!
 @abstract Application Group Identifier for Data Sharing

 @returns `NSString` value if data sharing is enabled, otherwise `nil`.
 */
+ (NSString *)applicationGroupIdentifierForDataSharing;

/*!
 @abstract Containing application bundle identifier.

 @returns `NSString` value if data sharing is enabled, otherwise `nil`.
 */
+ (NSString *)containingApplicationBundleIdentifierForDataSharing;

#if PARSE_IOS_ONLY

///--------------------------------------
/// @name Configuring UI Settings
///--------------------------------------

/*!
 @abstract Set whether to show offline messages when using a Parse view or view controller related classes.

 @param enabled Whether a `UIAlertView` should be shown when the device is offline
 and network access is required from a view or view controller.

 @deprecated This method has no effect.
 */
+ (void)offlineMessagesEnabled:(BOOL)enabled PARSE_DEPRECATED("This method is deprecated and has no effect.");

/*!
 @abstract Set whether to show an error message when using a Parse view or view controller related classes
 and a Parse error was generated via a query.

 @param enabled Whether a `UIAlertView` should be shown when an error occurs.

 @deprecated This method has no effect.
 */
+ (void)errorMessagesEnabled:(BOOL)enabled PARSE_DEPRECATED("This method is deprecated and has no effect.");

#endif

///--------------------------------------
/// @name Logging
///--------------------------------------

/*!
 @abstract Sets the level of logging to display.

 @discussion By default:
 - If running inside an app that was downloaded from iOS App Store - it is set to <PFLogLevelNone>
 - All other cases - it is set to <PFLogLevelWarning>

 @param logLevel Log level to set.
 @see PFLogLevel
 */
+ (void)setLogLevel:(PFLogLevel)logLevel;

/*!
 @abstract Log level that will be displayed.

 @discussion By default:
 - If running inside an app that was downloaded from iOS App Store - it is set to <PFLogLevelNone>
 - All other cases - it is set to <PFLogLevelWarning>

 @returns A <PFLogLevel> value.
 @see PFLogLevel
 */
+ (PFLogLevel)logLevel;

@end

PF_ASSUME_NONNULL_END
