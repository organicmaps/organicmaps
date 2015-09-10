/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFNullability.h>

@class PFObject;
@class PFUser;

///--------------------------------------
/// @name Version
///--------------------------------------

#define PARSE_VERSION @"1.8.2"

extern NSInteger const PARSE_API_VERSION;

///--------------------------------------
/// @name Platform
///--------------------------------------

#define PARSE_IOS_ONLY (TARGET_OS_IPHONE)
#define PARSE_OSX_ONLY (TARGET_OS_MAC && !(TARGET_OS_IPHONE))

extern NSString *const PF_NONNULL_S kPFDeviceType;

#if PARSE_IOS_ONLY
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

///--------------------------------------
/// @name Server
///--------------------------------------

extern NSString *const PF_NONNULL_S kPFParseServer;

///--------------------------------------
/// @name Cache Policies
///--------------------------------------

/*!
 `PFCachePolicy` specifies different caching policies that could be used with <PFQuery>.

 This lets you show data when the user's device is offline,
 or when the app has just started and network requests have not yet had time to complete.
 Parse takes care of automatically flushing the cache when it takes up too much space.

 @warning Cache policy could only be set when Local Datastore is not enabled.

 @see PFQuery
 */
typedef NS_ENUM(uint8_t, PFCachePolicy) {
    /*!
     @abstract The query does not load from the cache or save results to the cache.
     This is the default cache policy.
     */
    kPFCachePolicyIgnoreCache = 0,
    /*!
     @abstract The query only loads from the cache, ignoring the network.
     If there are no cached results, this causes a `NSError` with `kPFErrorCacheMiss` code.
     */
    kPFCachePolicyCacheOnly,
    /*!
     @abstract The query does not load from the cache, but it will save results to the cache.
     */
    kPFCachePolicyNetworkOnly,
    /*!
     @abstract The query first tries to load from the cache, but if that fails, it loads results from the network.
     If there are no cached results, this causes a `NSError` with `kPFErrorCacheMiss` code.
     */
    kPFCachePolicyCacheElseNetwork,
    /*!
     @abstract The query first tries to load from the network, but if that fails, it loads results from the cache.
     If there are no cached results, this causes a `NSError` with `kPFErrorCacheMiss` code.
     */
    kPFCachePolicyNetworkElseCache,
    /*!
     @abstract The query first loads from the cache, then loads from the network.
     The callback will be called twice - first with the cached results, then with the network results.
     Since it returns two results at different times, this cache policy cannot be used with synchronous or task methods.
     */
    kPFCachePolicyCacheThenNetwork
};

///--------------------------------------
/// @name Logging Levels
///--------------------------------------

/*!
 `PFLogLevel` enum specifies different levels of logging that could be used to limit or display more messages in logs.

 @see [Parse setLogLevel:]
 @see [Parse logLevel]
 */
typedef NS_ENUM(uint8_t, PFLogLevel) {
    /*!
     Log level that disables all logging.
     */
    PFLogLevelNone = 0,
    /*!
     Log level that if set is going to output error messages to the log.
     */
    PFLogLevelError = 1,
    /*!
     Log level that if set is going to output the following messages to log:
     - Errors
     - Warnings
     */
    PFLogLevelWarning = 2,
    /*!
     Log level that if set is going to output the following messages to log:
     - Errors
     - Warnings
     - Informational messages
     */
    PFLogLevelInfo = 3,
    /*!
     Log level that if set is going to output the following messages to log:
     - Errors
     - Warnings
     - Informational messages
     - Debug messages
     */
    PFLogLevelDebug = 4
};

///--------------------------------------
/// @name Errors
///--------------------------------------

extern NSString *const PF_NONNULL_S PFParseErrorDomain;

/*!
 `PFErrorCode` enum contains all custom error codes that are used as `code` for `NSError` for callbacks on all classes.

 These codes are used when `domain` of `NSError` that you receive is set to `PFParseErrorDomain`.
 */
typedef NS_ENUM(NSInteger, PFErrorCode) {
    /*!
     @abstract Internal server error. No information available.
     */
    kPFErrorInternalServer = 1,
    /*!
     @abstract The connection to the Parse servers failed.
     */
    kPFErrorConnectionFailed = 100,
    /*!
     @abstract Object doesn't exist, or has an incorrect password.
     */
    kPFErrorObjectNotFound = 101,
    /*!
     @abstract You tried to find values matching a datatype that doesn't
     support exact database matching, like an array or a dictionary.
     */
    kPFErrorInvalidQuery = 102,
    /*!
     @abstract Missing or invalid classname. Classnames are case-sensitive.
     They must start with a letter, and `a-zA-Z0-9_` are the only valid characters.
     */
    kPFErrorInvalidClassName = 103,
    /*!
     @abstract Missing object id.
     */
    kPFErrorMissingObjectId = 104,
    /*!
     @abstract Invalid key name. Keys are case-sensitive.
     They must start with a letter, and `a-zA-Z0-9_` are the only valid characters.
     */
    kPFErrorInvalidKeyName = 105,
    /*!
     @abstract Malformed pointer. Pointers must be arrays of a classname and an object id.
     */
    kPFErrorInvalidPointer = 106,
    /*!
     @abstract Malformed json object. A json dictionary is expected.
     */
    kPFErrorInvalidJSON = 107,
    /*!
     @abstract Tried to access a feature only available internally.
     */
    kPFErrorCommandUnavailable = 108,
    /*!
     @abstract Field set to incorrect type.
     */
    kPFErrorIncorrectType = 111,
    /*!
     @abstract Invalid channel name. A channel name is either an empty string (the broadcast channel)
     or contains only `a-zA-Z0-9_` characters and starts with a letter.
     */
    kPFErrorInvalidChannelName = 112,
    /*!
     @abstract Invalid device token.
     */
    kPFErrorInvalidDeviceToken = 114,
    /*!
     @abstract Push is misconfigured. See details to find out how.
     */
    kPFErrorPushMisconfigured = 115,
    /*!
     @abstract The object is too large.
     */
    kPFErrorObjectTooLarge = 116,
    /*!
     @abstract That operation isn't allowed for clients.
     */
    kPFErrorOperationForbidden = 119,
    /*!
     @abstract The results were not found in the cache.
     */
    kPFErrorCacheMiss = 120,
    /*!
     @abstract Keys in `NSDictionary` values may not include `$` or `.`.
     */
    kPFErrorInvalidNestedKey = 121,
    /*!
     @abstract Invalid file name.
     A file name can contain only `a-zA-Z0-9_.` characters and should be between 1 and 36 characters.
     */
    kPFErrorInvalidFileName = 122,
    /*!
     @abstract Invalid ACL. An ACL with an invalid format was saved. This should not happen if you use <PFACL>.
     */
    kPFErrorInvalidACL = 123,
    /*!
     @abstract The request timed out on the server. Typically this indicates the request is too expensive.
     */
    kPFErrorTimeout = 124,
    /*!
     @abstract The email address was invalid.
     */
    kPFErrorInvalidEmailAddress = 125,
    /*!
     A unique field was given a value that is already taken.
     */
    kPFErrorDuplicateValue = 137,
    /*!
     @abstract Role's name is invalid.
     */
    kPFErrorInvalidRoleName = 139,
    /*!
     @abstract Exceeded an application quota. Upgrade to resolve.
     */
    kPFErrorExceededQuota = 140,
    /*!
     @abstract Cloud Code script had an error.
     */
    kPFScriptError = 141,
    /*!
     @abstract Cloud Code validation failed.
     */
    kPFValidationError = 142,
    /*!
     @abstract Product purchase receipt is missing.
     */
    kPFErrorReceiptMissing = 143,
    /*!
     @abstract Product purchase receipt is invalid.
     */
    kPFErrorInvalidPurchaseReceipt = 144,
    /*!
     @abstract Payment is disabled on this device.
     */
    kPFErrorPaymentDisabled = 145,
    /*!
     @abstract The product identifier is invalid.
     */
    kPFErrorInvalidProductIdentifier = 146,
    /*!
     @abstract The product is not found in the App Store.
     */
    kPFErrorProductNotFoundInAppStore = 147,
    /*!
     @abstract The Apple server response is not valid.
     */
    kPFErrorInvalidServerResponse = 148,
    /*!
     @abstract Product fails to download due to file system error.
     */
    kPFErrorProductDownloadFileSystemFailure = 149,
    /*!
     @abstract Fail to convert data to image.
     */
    kPFErrorInvalidImageData = 150,
    /*!
     @abstract Unsaved file.
     */
    kPFErrorUnsavedFile = 151,
    /*!
     @abstract Fail to delete file.
     */
    kPFErrorFileDeleteFailure = 153,
    /*!
     @abstract Application has exceeded its request limit.
     */
    kPFErrorRequestLimitExceeded = 155,
    /*!
     @abstract Invalid event name.
     */
    kPFErrorInvalidEventName = 160,
    /*!
     @abstract Username is missing or empty.
     */
    kPFErrorUsernameMissing = 200,
    /*!
     @abstract Password is missing or empty.
     */
    kPFErrorUserPasswordMissing = 201,
    /*!
     @abstract Username has already been taken.
     */
    kPFErrorUsernameTaken = 202,
    /*!
     @abstract Email has already been taken.
     */
    kPFErrorUserEmailTaken = 203,
    /*!
     @abstract The email is missing, and must be specified.
     */
    kPFErrorUserEmailMissing = 204,
    /*!
     @abstract A user with the specified email was not found.
     */
    kPFErrorUserWithEmailNotFound = 205,
    /*!
     @abstract The user cannot be altered by a client without the session.
     */
    kPFErrorUserCannotBeAlteredWithoutSession = 206,
    /*!
     @abstract Users can only be created through sign up.
     */
    kPFErrorUserCanOnlyBeCreatedThroughSignUp = 207,
    /*!
     @abstract An existing Facebook account already linked to another user.
     */
    kPFErrorFacebookAccountAlreadyLinked = 208,
    /*!
     @abstract An existing account already linked to another user.
     */
    kPFErrorAccountAlreadyLinked = 208,
    /*!
     Error code indicating that the current session token is invalid.
     */
    kPFErrorInvalidSessionToken = 209,
    kPFErrorUserIdMismatch = 209,
    /*!
     @abstract Facebook id missing from request.
     */
    kPFErrorFacebookIdMissing = 250,
    /*!
     @abstract Linked id missing from request.
     */
    kPFErrorLinkedIdMissing = 250,
    /*!
     @abstract Invalid Facebook session.
     */
    kPFErrorFacebookInvalidSession = 251,
    /*!
     @abstract Invalid linked session.
     */
    kPFErrorInvalidLinkedSession = 251,
};

///--------------------------------------
/// @name Blocks
///--------------------------------------

typedef void (^PFBooleanResultBlock)(BOOL succeeded, NSError *PF_NULLABLE_S error);
typedef void (^PFIntegerResultBlock)(int number, NSError *PF_NULLABLE_S error);
typedef void (^PFArrayResultBlock)(NSArray *PF_NULLABLE_S objects, NSError *PF_NULLABLE_S error);
typedef void (^PFObjectResultBlock)(PFObject *PF_NULLABLE_S object,  NSError *PF_NULLABLE_S error);
typedef void (^PFSetResultBlock)(NSSet *PF_NULLABLE_S channels, NSError *PF_NULLABLE_S error);
typedef void (^PFUserResultBlock)(PFUser *PF_NULLABLE_S user, NSError *PF_NULLABLE_S error);
typedef void (^PFDataResultBlock)(NSData *PF_NULLABLE_S data, NSError *PF_NULLABLE_S error);
typedef void (^PFDataStreamResultBlock)(NSInputStream *PF_NULLABLE_S stream, NSError *PF_NULLABLE_S error);
typedef void (^PFStringResultBlock)(NSString *PF_NULLABLE_S string, NSError *PF_NULLABLE_S error);
typedef void (^PFIdResultBlock)(PF_NULLABLE_S id object, NSError *PF_NULLABLE_S error);
typedef void (^PFProgressBlock)(int percentDone);

///--------------------------------------
/// @name Deprecated Macros
///--------------------------------------

#ifndef PARSE_DEPRECATED
#  ifdef __deprecated_msg
#    define PARSE_DEPRECATED(_MSG) __deprecated_msg(_MSG)
#  else
#    ifdef __deprecated
#      define PARSE_DEPRECATED(_MSG) __attribute__((deprecated))
#    else
#      define PARSE_DEPRECATED(_MSG)
#    endif
#  endif
#endif

///--------------------------------------
/// @name Extensions Macros
///--------------------------------------

#ifndef PF_EXTENSION_UNAVAILABLE
#  if PARSE_IOS_ONLY
#    ifdef NS_EXTENSION_UNAVAILABLE_IOS
#      define PF_EXTENSION_UNAVAILABLE(_msg) NS_EXTENSION_UNAVAILABLE_IOS(_msg)
#    else
#      define PF_EXTENSION_UNAVAILABLE(_msg)
#    endif
#  else
#    ifdef NS_EXTENSION_UNAVAILABLE_MAC
#      define PF_EXTENSION_UNAVAILABLE(_msg) NS_EXTENSION_UNAVAILABLE_MAC(_msg)
#    else
#      define PF_EXTENSION_UNAVAILABLE(_msg)
#    endif
#  endif
#endif
