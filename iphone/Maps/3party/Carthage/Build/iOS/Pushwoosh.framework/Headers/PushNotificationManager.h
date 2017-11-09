//
//  PushNotificationManager.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2014
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>
#import <UserNotifications/UserNotifications.h>
#endif

#define PUSHWOOSH_VERSION @"5.4.0"


@class PushNotificationManager;

#if TARGET_OS_IPHONE
@class CLLocation;
#endif

typedef void (^PushwooshGetTagsHandler)(NSDictionary *tags);
typedef void (^PushwooshErrorHandler)(NSError *error);

/**
 `PushNotificationDelegate` protocol defines the methods that can be implemented in the delegate of the `PushNotificationManager` class' singleton object.
 These methods provide information about the key events for push notification manager such as registering with APS services, receiving push notifications or working with the received notification.
 These methods implementation allows to react on these events properly.
 */
@protocol PushNotificationDelegate

@optional
/**
 Tells the delegate that the application has registered with Apple Push Service (APS) successfully.
 
 @param token A token used for identifying the device with APS.
 */
- (void)onDidRegisterForRemoteNotificationsWithDeviceToken:(NSString *)token;

/**
 Sent to the delegate when Apple Push Service (APS) could not complete the registration process successfully.
 
 @param error An NSError object encapsulating the information about the reason of the registration failure. Within this method you can define application's behaviour in case of registration failure.
 */
- (void)onDidFailToRegisterForRemoteNotificationsWithError:(NSError *)error;

/**
 Tells the delegate that the push manager has received a remote notification.
 
 @param pushManager The push manager that received the remote notification.
 @param pushNotification A dictionary that contains information referring to the remote notification, potentially including a badge number for the application icon, an alert sound, an alert message to display to the user, a notification identifier, and custom data.
 The provider originates it as a JSON-defined dictionary that iOS converts to an NSDictionary object; the dictionary may contain only property-list objects plus NSNull.
@param onStart If the application was not foreground when the push notification was received, the application will be opened with this parameter equal to `YES`, otherwise the parameter will be `NO`.
 */
- (void)onPushReceived:(PushNotificationManager *)pushManager withNotification:(NSDictionary *)pushNotification onStart:(BOOL)onStart;

/**
 Tells the delegate that the user has pressed OK on the push notification.
 IMPORTANT: This method is used for backwards compatibility and is deprecated. Please use the `onPushAccepted:withNotification:onStart:` method instead
 
 @param pushManager The push manager that received the remote notification.
 @param pushNotification A dictionary that contains information referring to the remote notification, potentially including a badge number for the application icon, an alert sound, an alert message to display to the user, a notification identifier, and custom data.
 The provider originates it as a JSON-defined dictionary that iOS converts to an NSDictionary object; the dictionary may contain only property-list objects plus NSNull.
 Push dictionary sample:
 
 {
 aps =     {
 alert = "Some text.";
 sound = default;
 };
 p = 1pb;
 }
 
 */
- (void)onPushAccepted:(PushNotificationManager *)pushManager withNotification:(NSDictionary *)pushNotification DEPRECATED_ATTRIBUTE;

/**
 Tells the delegate that the user has pressed on the push notification banner.
 
 @param pushManager The push manager that received the remote notification.
 @param pushNotification A dictionary that contains information about the remote notification, potentially including a badge number for the application icon, an alert sound, an alert message to display to the user, a notification identifier, and custom data.
 The provider originates it as a JSON-defined dictionary that iOS converts to an NSDictionary object; the dictionary may contain only property-list objects plus NSNull.
 Push dictionary sample:
 
 {
 aps =     {
 alert = "Some text.";
 sound = default;
 };
 p = 1pb;
 }
 
 @param onStart If the application was not foreground when the push notification was received, the application will be opened with this parameter equal to `YES`, otherwise the parameter will be `NO`.
 */
- (void)onPushAccepted:(PushNotificationManager *)pushManager withNotification:(NSDictionary *)pushNotification onStart:(BOOL)onStart;

/**
 Tells the delegate that the push manager has received tags from the server.
 
 @param tags Dictionary representation of received tags.
 Dictionary example:
 
 {
 Country = ru;
 Language = ru;
 }
 
 */
- (void)onTagsReceived:(NSDictionary *)tags;

/**
 Sent to the delegate when push manager could not complete the tags receiving process successfully.
 
 @param error An NSError object that encapsulates information why receiving tags did not succeed.
 */
- (void)onTagsFailedToReceive:(NSError *)error;

/**
 Tells the delegate that In-App with specified code has been closed
 
 @param code In-App code
 */
- (void)onInAppClosed:(NSString *)code;

/**
 Tells the delegate that In-App with specified code has been displayed
 
 @param code In-App code
 */
- (void)onInAppDisplayed:(NSString *)code;

@end

/**
 `PWTags` class encapsulates the methods for creating tags parameters for sending them to the server.
 */
@interface PWTags : NSObject

/**
 Creates a dictionary for incrementing/decrementing a numeric tag on the server.
 
 Example:
 
	NSDictionary *tags = [NSDictionary dictionaryWithObjectsAndKeys:
 aliasField.text, @"Alias",
 [NSNumber numberWithInt:[favNumField.text intValue]], @"FavNumber",
 [PWTags incrementalTagWithInteger:5], @"price",
 nil];
 
	[[PushNotificationManager pushManager] setTags:tags];
 
 @param delta Difference that needs to be applied to the tag's counter.
 
 @return Dictionary, that needs to be sent as the value for the tag
 */
+ (NSDictionary *)incrementalTagWithInteger:(NSInteger)delta;

@end

/**
 `PushNotificationManager` class offers access to the singletone-instance of the push manager responsible for registering the device with the APS servers, receiving and processing push notifications.
 */
@interface PushNotificationManager : NSObject {
}

/**
 Pushwoosh Application ID. Usually retrieved automatically from Info.plist parameter `Pushwoosh_APPID`
 */
@property (nonatomic, copy, readonly) NSString *appCode;

/**
 Application name. Usually retrieved automatically from Info.plist bundle name (CFBundleDisplayName). Could be used to override bundle name. In addition could be set in Info.plist as `Pushwoosh_APPNAME` parameter.
 */
@property (nonatomic, copy, readonly) NSString *appName;

/**
 `PushNotificationDelegate` protocol delegate that would receive the information about events for push notification manager such as registering with APS services, receiving push notifications or working with the received notification.
 Pushwoosh Runtime sets it to ApplicationDelegate by default
 */
@property (nonatomic, weak) NSObject<PushNotificationDelegate> *delegate;

#if TARGET_OS_IPHONE

/**
 Show push notifications alert when push notification is received while the app is running, default is `YES`
 */
@property (nonatomic, assign) BOOL showPushnotificationAlert;

#endif

/**
 Returns push notification payload if the app was started in response to push notification or null otherwise
 */
@property (nonatomic, copy, readonly) NSDictionary *launchNotification;

#if TARGET_OS_IPHONE

/**
 Returns UNUserNotificationCenterDelegate that handles foreground push notifications on iOS10
 */
@property (nonatomic, strong, readonly) id<UNUserNotificationCenterDelegate> notificationCenterDelegate;

#else

@property (nonatomic, strong, readonly) id<NSUserNotificationCenterDelegate> notificationCenterDelegate;

#endif

/**
 Initializes PushNotificationManager. Usually called by Pushwoosh Runtime internally.
 @param appCode Pushwoosh App ID.
 @param appName Application name.
 */
+ (void)initializeWithAppCode:(NSString *)appCode appName:(NSString *)appName;

/**
 Returns an object representing the current push manager.
 
 @return A singleton object that represents the push manager.
 */
+ (PushNotificationManager *)pushManager;

/**
 Registers for push notifications. By default registeres for "UIRemoteNotificationTypeBadge | UIRemoteNotificationTypeSound | UIRemoteNotificationTypeAlert" flags.
 Automatically detects if you have "newsstand-content" in "UIBackgroundModes" and adds "UIRemoteNotificationTypeNewsstandContentAvailability" flag.
 */
- (void)registerForPushNotifications;

/**
 Unregisters from push notifications. You should call this method in rare circumstances only, such as when a new version of the app drops support for remote notifications. Users can temporarily prevent apps from receiving remote notifications through the Notifications section of the Settings app. Apps unregistered through this method can always re-register.
 */
- (void)unregisterForPushNotifications;

/**
 Deprecated. Use initializeWithAppCode:appName: method class
 */
- (instancetype)initWithApplicationCode:(NSString *)appCode appName:(NSString *)appName __attribute__((deprecated));

#if TARGET_OS_IPHONE

/**
 Deprecated. Use initializeWithAppCode:appName: method class
 */
- (id)initWithApplicationCode:(NSString *)appCode navController:(UIViewController *)navController appName:(NSString *)appName __attribute__((deprecated));

/**
 Sends geolocation to the server for GeoFencing push technology. Called internally, please use `startLocationTracking` and `stopLocationTracking` functions.
 
 @param location Location to be sent.
 */
- (void)sendLocation:(CLLocation *)location;

/**
 Start location tracking.
 */
- (void)startLocationTracking;

/**
 Stops location tracking
 */
- (void)stopLocationTracking;

#endif

/**
 Send tags to server. Tag names have to be created in the Pushwoosh Control Panel. Possible tag types: Integer, String, Incremental (integer only), List tags (array of values).
 
 Example:
 
 NSDictionary *tags = [NSDictionary dictionaryWithObjectsAndKeys:
 aliasField.text, @"Alias",
 [NSNumber numberWithInt:[favNumField.text intValue]], @"FavNumber",
 [PWTags incrementalTagWithInteger:5], @"price",
 [NSArray arrayWithObjects:@"Item1", @"Item2", @"Item3", nil], @"List",
 nil];
	
 [[PushNotificationManager pushManager] setTags:tags];
 
 @param tags Dictionary representation of tags to send.
 */
- (void)setTags:(NSDictionary *)tags;

/**
 Send tags to server with completion block. If setTags succeeds competion is called with nil argument. If setTags fails completion is called with error.
 */
- (void)setTags:(NSDictionary *)tags withCompletion:(void (^)(NSError *error))completion;

/**
 Get tags from the server. Calls delegate method `onTagsReceived:` or `onTagsFailedToReceive:` depending on the results.
 */
- (void)loadTags;

/**
 Get tags from server. Calls delegate method if exists and handler (block).
 
 @param successHandler The block is executed on the successful completion of the request. This block has no return value and takes one argument: the dictionary representation of the recieved tags.
 Example of the dictionary representation of the received tags:
 
 {
 Country = ru;
 Language = ru;
 }
 
 @param errorHandler The block is executed on the unsuccessful completion of the request. This block has no return value and takes one argument: the error that occurred during the request.
 */
- (void)loadTags:(PushwooshGetTagsHandler)successHandler error:(PushwooshErrorHandler)errorHandler;

/**
 Informs the Pushwoosh about the app being launched. Usually called internally by SDK Runtime.
 */
- (void)sendAppOpen;

/**
 Sends current badge value to server. Called internally by SDK Runtime when `UIApplication` `setApplicationBadgeNumber:` is set. This function is used for "auto-incremeting" badges to work.
 This way Pushwoosh server can know what current badge value is set for the application.
 
 @param badge Current badge value.
 */
- (void)sendBadges:(NSInteger)badge;

#if TARGET_OS_IPHONE
/**
 Sends in-app purchases to Pushwoosh. Use in paymentQueue:updatedTransactions: payment queue method (see example).
 
 Example:
 
 - (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
 [[PushNotificationManager pushManager] sendSKPaymentTransactions:transactions];
 }
 
 @param transactions Array of SKPaymentTransaction items as received in the payment queue.
 */
- (void)sendSKPaymentTransactions:(NSArray *)transactions;
#endif

/**
 Tracks individual in-app purchase. See recommended `sendSKPaymentTransactions:` method.
 
 @param productIdentifier purchased product ID
 @param price price for the product
 @param currencyCode currency of the price (ex: @"USD")
 @param date time of the purchase (ex: [NSDate now])
 */
- (void)sendPurchase:(NSString *)productIdentifier withPrice:(NSDecimalNumber *)price currencyCode:(NSString *)currencyCode andDate:(NSDate *)date;

/**
 Gets current push token.
 
 @return Current push token. May be nil if no push token is available yet.
 */
- (NSString *)getPushToken;

/**
 Gets HWID. Unique device identifier that used in all API calls with Pushwoosh.
 This is identifierForVendor for iOS >= 7.
 
 @return Unique device identifier.
 */
- (NSString *)getHWID;

- (void)handlePushRegistration:(NSData *)devToken;
- (void)handlePushRegistrationString:(NSString *)deviceID;

//internal
- (void)handlePushRegistrationFailure:(NSError *)error;

//if the push is received while the app is running. internal
- (BOOL)handlePushReceived:(NSDictionary *)userInfo;

/**
 Gets APN payload from push notifications dictionary.
 
 Example:
 
 - (void) onPushAccepted:(PushNotificationManager *)pushManager withNotification:(NSDictionary *)pushNotification onStart:(BOOL)onStart {
 NSDictionary * apnPayload = [[PushNotificationsManager pushManager] getApnPayload:pushNotification];
 NSLog(@"%@", apnPayload);
 }
 
 For Push dictionary sample:
 
 {
 aps =     {
 alert = "Some text.";
 sound = default;
 };
 p = 1pb;
 }
 
 Result is:
 
 {
 alert = "Some text.";
 sound = default;
 };
 
 @param pushNotification Push notifications dictionary as received in `onPushAccepted: withNotification: onStart:`
 */
- (NSDictionary *)getApnPayload:(NSDictionary *)pushNotification;

/**
 Gets custom JSON string data from push notifications dictionary as specified in Pushwoosh Control Panel.
 
 Example:
 
 - (void) onPushAccepted:(PushNotificationManager *)pushManager withNotification:(NSDictionary *)pushNotification onStart:(BOOL)onStart {
 NSString * customData = [[PushNotificationsManager pushManager] getCustomPushData:pushNotification];
 NSLog(@"%@", customData);
 }
 
 @param pushNotification Push notifications dictionary as received in `onPushAccepted: withNotification: onStart:`
 */
- (NSString *)getCustomPushData:(NSDictionary *)pushNotification;

/**
 The same as getCustomPushData but returns NSDictionary rather than JSON string (converts JSON string into NSDictionary).
 */
- (NSDictionary *)getCustomPushDataAsNSDict:(NSDictionary *)pushNotification;

/**
 Returns dictionary with enabled remove notificaton types.
 Example enabled push:
 {
	enabled = 1;
	pushAlert = 1;
	pushBadge = 1;
	pushSound = 1;
	type = 7;
 }
 
 where "type" field is UIUserNotificationType
 
 Disabled push:
 {
	enabled = 1;
	pushAlert = 0;
	pushBadge = 0;
	pushSound = 0;
	type = 0;
 }
 
 Note: In the latter example "enabled" field means that device can receive push notification but could not display alerts (ex: silent push)
 */
+ (NSMutableDictionary *)getRemoteNotificationStatus;

/**
 Clears the notifications from the notification center.
 */
+ (void)clearNotificationCenter;

/**
 Set User indentifier. This could be Facebook ID, username or email, or any other user ID.
 This allows data and events to be matched across multiple user devices.
 
 Deprecated. Use PWInAppManager setUserId method instead
 */
- (void)setUserId:(NSString *)userId __attribute__ ((deprecated));;

/**
 Move all events from oldUserId to newUserId if doMerge is true. If doMerge is false all events for oldUserId are removed.
 
 @param oldUserId source user
 @param newUserId destination user
 @param doMerge if false all events for oldUserId are removed, if true all events for oldUserId are moved to newUserId
 @param completion callback
 
 Deprecated. Use PWInAppManager mergeUserId method instead
 */
- (void)mergeUserId:(NSString *)oldUserId to:(NSString *)newUserId doMerge:(BOOL)doMerge completion:(void (^)(NSError *error))completion __attribute__ ((deprecated));

/**
 Post events for In-App Messages. This can trigger In-App message display as specified in Pushwoosh Control Panel.
 
 Example:
 
 [[PushNotificationManager pushManager] setUserId:@"96da2f590cd7246bbde0051047b0d6f7"];
 [[PushNotificationManager pushManager] postEvent:@"buttonPressed" withAttributes:@{ @"buttonNumber" : @"4", @"buttonLabel" : @"Banner" } completion:nil];
 
 @param event name of the event
 @param attributes NSDictionary of event attributes
 @param completion function to call after posting event
 
 Deprecated. Use PWInAppManager postEvent method instead
 */
- (void)postEvent:(NSString *)event withAttributes:(NSDictionary *)attributes completion:(void (^)(NSError *error))completion __attribute__ ((deprecated));

/**
 See `postEvent:withAttributes:completion:`
 
 Deprecated. Use PWInAppManager postEvent method instead
 */
- (void)postEvent:(NSString *)event withAttributes:(NSDictionary *)attributes __attribute__ ((deprecated));

@end
