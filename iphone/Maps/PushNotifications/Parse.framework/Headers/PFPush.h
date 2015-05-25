//
//  PFPush.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Parse/PFConstants.h>

@class BFTask;
@class PFQuery;

/*!
 The `PFPush` class defines a push notification that can be sent from a client device.

 The preferred way of modifying or retrieving channel subscriptions is to use
 the <PFInstallation> class, instead of the class methods in `PFPush`.

 This class is currently for iOS only. Parse does not handle Push Notifications
 to Parse applications running on OS X. Push Notifications can be sent from OS X
 applications via Cloud Code or the REST API to push-enabled devices (e.g. iOS or Android).
 */
@interface PFPush : NSObject <NSCopying>

///--------------------------------------
/// @name Creating a Push Notification
///--------------------------------------

+ (PFPush *)push;

///--------------------------------------
/// @name Configuring a Push Notification
///--------------------------------------

/*!
 @abstract Sets the channel on which this push notification will be sent.

 @param channel The channel to set for this push.
 The channel name must start with a letter and contain only letters, numbers, dashes, and underscores.
 */
- (void)setChannel:(NSString *)channel;

/*!
 @abstract Sets the array of channels on which this push notification will be sent.

 @param channels The array of channels to set for this push.
 Each channel name must start with a letter and contain only letters, numbers, dashes, and underscores.
 */
- (void)setChannels:(NSArray *)channels;

/*!
 @abstract Sets an installation query to which this push notification will be sent.

 @discussion The query should be created via <[PFInstallation query]> and should not specify a skip, limit, or order.

 @param query The installation query to set for this push.
 */
- (void)setQuery:(PFQuery *)query;

/*!
 @abstract Sets an alert message for this push notification.

 @warning This will overwrite any data specified in setData.

 @param message The message to send in this push.
 */
- (void)setMessage:(NSString *)message;

/*!
 @abstract Sets an arbitrary data payload for this push notification.

 @discussion See the guide for information about the dictionary structure.

 @warning This will overwrite any data specified in setMessage.

 @param data The data to send in this push.
 */
- (void)setData:(NSDictionary *)data;

/*!
 @abstract Sets whether this push will go to Android devices.

 @param pushToAndroid Whether this push will go to Android devices.

 @deprecated Please use a `[PFInstallation query]` with a constraint on deviceType instead.
 */
- (void)setPushToAndroid:(BOOL)pushToAndroid PARSE_DEPRECATED("Please use a [PFInstallation query] with a constraint on deviceType.");

/*!
 @abstract Sets whether this push will go to iOS devices.

 @param pushToIOS Whether this push will go to iOS devices.

 @deprecated Please use a `[PFInstallation query]` with a constraint on deviceType instead.
 */
- (void)setPushToIOS:(BOOL)pushToIOS PARSE_DEPRECATED("Please use a [PFInstallation query] with a constraint on deviceType.");

/*!
 @abstract Sets the expiration time for this notification.

 @discussion The notification will be sent to devices which are either online
 at the time the notification is sent, or which come online before the expiration time is reached.
 Because device clocks are not guaranteed to be accurate,
 most applications should instead use <expireAfterTimeInterval:>.

 @see expireAfterTimeInterval:

 @param date The time at which the notification should expire.
 */
- (void)expireAtDate:(NSDate *)date;

/*!
 @abstract Sets the time interval after which this notification should expire.

 @discussion This notification will be sent to devices which are either online at
 the time the notification is sent, or which come online within the given
 time interval of the notification being received by Parse's server.
 An interval which is less than or equal to zero indicates that the
 message should only be sent to devices which are currently online.

 @param timeInterval The interval after which the notification should expire.
 */
- (void)expireAfterTimeInterval:(NSTimeInterval)timeInterval;

/*!
 @abstract Clears both expiration values, indicating that the notification should never expire.
 */
- (void)clearExpiration;

///--------------------------------------
/// @name Sending Push Notifications
///--------------------------------------

/*!
 @abstract *Synchronously* send a push message to a channel.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param message The message to send.
 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns whether the send succeeded.
 */
+ (BOOL)sendPushMessageToChannel:(NSString *)channel
                     withMessage:(NSString *)message
                           error:(NSError **)error;

/*!
 @abstract *Asynchronously* send a push message to a channel.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param message The message to send.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)sendPushMessageToChannelInBackground:(NSString *)channel
                                     withMessage:(NSString *)message;

/*!
 @abstract *Asynchronously* sends a push message to a channel and calls the given block.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param message The message to send.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)sendPushMessageToChannelInBackground:(NSString *)channel
                                 withMessage:(NSString *)message
                                       block:(PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* send a push message to a channel.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param message The message to send.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)sendPushMessageToChannelInBackground:(NSString *)channel
                                 withMessage:(NSString *)message
                                      target:(id)target
                                    selector:(SEL)selector;

/*!
 @abstract Send a push message to a query.

 @param query The query to send to. The query must be a <PFInstallation> query created with <[PFInstallation query]>.
 @param message The message to send.
 @param error Pointer to an NSError that will be set if necessary.

 @returns Returns whether the send succeeded.
 */
+ (BOOL)sendPushMessageToQuery:(PFQuery *)query
                   withMessage:(NSString *)message
                         error:(NSError **)error;

/*!
 @abstract *Asynchronously* send a push message to a query.

 @param query The query to send to. The query must be a <PFInstallation> query created with <[PFInstallation query]>.
 @param message The message to send.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)sendPushMessageToQueryInBackground:(PFQuery *)query
                                   withMessage:(NSString *)message;

/*!
 @abstract *Asynchronously* sends a push message to a query and calls the given block.

 @param query The query to send to. The query must be a PFInstallation query
 created with [PFInstallation query].
 @param message The message to send.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)sendPushMessageToQueryInBackground:(PFQuery *)query
                               withMessage:(NSString *)message
                                     block:(PFBooleanResultBlock)block;

/*!
 @abstract *Synchronously* send this push message.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns whether the send succeeded.
 */
- (BOOL)sendPush:(NSError **)error;

/*!
 @abstract *Asynchronously* send this push message.
 @returns The task, that encapsulates the work being done.
 */
- (BFTask *)sendPushInBackground;

/*!
 @abstract *Asynchronously* send this push message and executes the given callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)sendPushInBackgroundWithBlock:(PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* send this push message and calls the given callback.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
- (void)sendPushInBackgroundWithTarget:(id)target selector:(SEL)selector;

/*!
 @abstract *Synchronously* send a push message with arbitrary data to a channel.

 @discussion See the guide for information about the dictionary structure.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param data The data to send.
 @param error Pointer to an NSError that will be set if necessary.

 @returns Returns whether the send succeeded.
 */
+ (BOOL)sendPushDataToChannel:(NSString *)channel
                     withData:(NSDictionary *)data
                        error:(NSError **)error;

/*!
 @abstract *Asynchronously* send a push message with arbitrary data to a channel.

 @discussion See the guide for information about the dictionary structure.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param data The data to send.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)sendPushDataToChannelInBackground:(NSString *)channel
                                     withData:(NSDictionary *)data;

/*!
 @abstract Asynchronously sends a push message with arbitrary data to a channel and calls the given block.

 @discussion See the guide for information about the dictionary structure.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param data The data to send.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)sendPushDataToChannelInBackground:(NSString *)channel
                                 withData:(NSDictionary *)data
                                    block:(PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* send a push message with arbitrary data to a channel.

 @discussion See the guide for information about the dictionary structure.

 @param channel The channel to send to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param data The data to send.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)sendPushDataToChannelInBackground:(NSString *)channel
                                 withData:(NSDictionary *)data
                                   target:(id)target
                                 selector:(SEL)selector;

/*!
 @abstract *Synchronously* send a push message with arbitrary data to a query.

 @discussion See the guide for information about the dictionary structure.

 @param query The query to send to. The query must be a <PFInstallation> query
 created with <[PFInstallation query]>.
 @param data The data to send.
 @param error Pointer to an NSError that will be set if necessary.

 @returns Returns whether the send succeeded.
 */
+ (BOOL)sendPushDataToQuery:(PFQuery *)query
                   withData:(NSDictionary *)data
                      error:(NSError **)error;

/*!
 @abstract Asynchronously send a push message with arbitrary data to a query.

 @discussion See the guide for information about the dictionary structure.

 @param query The query to send to. The query must be a <PFInstallation> query
 created with <[PFInstallation query]>.
 @param data The data to send.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)sendPushDataToQueryInBackground:(PFQuery *)query
                                   withData:(NSDictionary *)data;

/*!
 @abstract *Asynchronously* sends a push message with arbitrary data to a query and calls the given block.

 @discussion See the guide for information about the dictionary structure.

 @param query The query to send to. The query must be a <PFInstallation> query
 created with <[PFInstallation query]>.
 @param data The data to send.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)sendPushDataToQueryInBackground:(PFQuery *)query
                               withData:(NSDictionary *)data
                                  block:(PFBooleanResultBlock)block;

///--------------------------------------
/// @name Handling Notifications
///--------------------------------------

/*!
 @abstract A default handler for push notifications while the app is active that
 could be used to mimic the behavior of iOS push notifications while the app is backgrounded or not running.

 @discussion Call this from `application:didReceiveRemoteNotification:`.

 @param userInfo The userInfo dictionary you get in `appplication:didReceiveRemoteNotification:`.
 */
+ (void)handlePush:(NSDictionary *)userInfo;

///--------------------------------------
/// @name Managing Channel Subscriptions
///--------------------------------------

/*!
 @abstract Store the device token locally for push notifications.

 @discussion Usually called from you main app delegate's `didRegisterForRemoteNotificationsWithDeviceToken:`.

 @param deviceToken Either as an `NSData` straight from `application:didRegisterForRemoteNotificationsWithDeviceToken:`
 or as an `NSString` if you converted it yourself.
 */
+ (void)storeDeviceToken:(id)deviceToken;

/*!
 @abstract *Synchronously* get all the channels that this device is subscribed to.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns an `NSSet` containing all the channel names this device is subscribed to.
 */
+ (NSSet *)getSubscribedChannels:(NSError **)error;

/*!
 @abstract *Asynchronously* get all the channels that this device is subscribed to.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)getSubscribedChannelsInBackground;

/*!
 @abstract *Asynchronously* get all the channels that this device is subscribed to.
 @param block The block to execute.
 It should have the following argument signature: `^(NSSet *channels, NSError *error)`.
 */
+ (void)getSubscribedChannelsInBackgroundWithBlock:(PFSetResultBlock)block;

/*
 @abstract *Asynchronously* get all the channels that this device is subscribed to.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSSet *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 */
+ (void)getSubscribedChannelsInBackgroundWithTarget:(id)target
                                           selector:(SEL)selector;

/*!
 @abstract *Synchrnously* subscribes the device to a channel of push notifications.

 @param channel The channel to subscribe to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns whether the subscribe succeeded.
 */
+ (BOOL)subscribeToChannel:(NSString *)channel error:(NSError **)error;

/*!
 @abstract *Asynchronously* subscribes the device to a channel of push notifications.

 @param channel The channel to subscribe to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)subscribeToChannelInBackground:(NSString *)channel;

/*!
 @abstract *Asynchronously* subscribes the device to a channel of push notifications and calls the given block.

 @param channel The channel to subscribe to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)subscribeToChannelInBackground:(NSString *)channel
                                 block:(PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* subscribes the device to a channel of push notifications and calls the given callback.

 @param channel The channel to subscribe to. The channel name must start with
 a letter and contain only letters, numbers, dashes, and underscores.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)subscribeToChannelInBackground:(NSString *)channel
                                target:(id)target
                              selector:(SEL)selector;

/*!
 @abstract *Synchronously* unsubscribes the device to a channel of push notifications.

 @param channel The channel to unsubscribe from.
 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns whether the unsubscribe succeeded.
 */
+ (BOOL)unsubscribeFromChannel:(NSString *)channel error:(NSError **)error;

/*!
 @abstract *Asynchronously* unsubscribes the device from a channel of push notifications.

 @param channel The channel to unsubscribe from.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)unsubscribeFromChannelInBackground:(NSString *)channel;

/*!
 @abstract *Asynchronously* unsubscribes the device from a channel of push notifications and calls the given block.

 @param channel The channel to unsubscribe from.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)unsubscribeFromChannelInBackground:(NSString *)channel
                                     block:(PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* unsubscribes the device from a channel of push notifications and calls the given callback.

 @param channel The channel to unsubscribe from.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)unsubscribeFromChannelInBackground:(NSString *)channel
                                    target:(id)target
                                  selector:(SEL)selector;

@end
