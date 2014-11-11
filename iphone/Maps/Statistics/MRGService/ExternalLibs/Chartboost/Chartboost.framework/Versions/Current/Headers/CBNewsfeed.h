/*
 * CBNewsfeed.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

/*! @abstract CBStory forward class declaration. */
@class CBStory;

/*! @abstract CBStory forward protocol declaration. */
@protocol CBNewsfeedDelegate;

/*!
 @class CBNewsfeed
 
 @abstract
 Provide methods to control the Chartboost Newsfeed.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBNewsfeed : NSObject


#pragma mark - CBNewsfeed Initialzation

/*!
 @abstract
 Start the Newsfeed with the given delegate.  You must call startWithAppId:appSignature:delegate: in the
 Chartboost SDK main class before this can be used.
 
 @param delegate The delegate instance to receive CBNewsfeed SDK callbacks.
 
 @discussion Call this in application:didFinishLaunchingWithOptions: to start getting
 Newsfeed data from the Chartboost API servers.
 */
+ (void)startWithDelegate:(id<CBNewsfeedDelegate>)delegate;


#pragma mark - CBNewsfeed Data Methods

/*!
 @abstract
 Get CBStory objects currently stored on the device.
 
 @return Array of CBStory objects.
 
 @discussion Only returns CBStory objects on the device local storage.
 Will not trigger a request to the Chartboost API servers.
 */
+ (NSArray *)getMessages;

/*!
 @abstract
 Get CBStory object currently stored on the device with the given message ID.
 
 @param messageId The unique messageID for the CBStory.
 
 @return CBStory object.
 
 @discussion Only returns CBStory object on the device local storage.
 Will not trigger a request to the Chartboost API servers.
 */
+ (CBStory *)getMessage:(NSString *)messageId;

/*!
 @abstract
 Get the total number of messages currently on the device.
 
 @return NSUInteger Total number of local messages or 0.
 
 @discussion Only returns CBStory object count on the device local storage.
 Will not trigger a request to the Chartboost API servers. This returns all messages regardless
 if they have been read or not.
 */
+ (NSUInteger)messageCount;

/*!
 @abstract
 Get the number of unread messages currently on the device.
 
 @return NSUInteger Total number of unread local messages or 0.
 
 @discussion Only returns CBStory object count on the device local storage.
 Will not trigger a request to the Chartboost API servers.
 */
+ (NSUInteger)unreadMessageCount;

/*!
 @abstract
 Retrieve CBStory data from the Chartboost API server.
 
 @discussion Retrieve CBStory data from the Chartboost API server and will trigger
 the delegate method didGetNewMessages: if new messages are found.
 
 On success it will trigger the didRetrieveMessages delegate and on failure the didFailToRetrieveMessages:
 delegate.  On success it is left to the developer to check the local storage for changes to message objects.
 */
+ (void)retrieveMessages;

#pragma mark - CBNewsfeed Display Methods

/*!
 @abstract
 Check if the NewsfeedUI is visible.
 
 @return YES if the UI is visible, NO if not.
 
 @discussion Calls [CBNewsfeedUIProtocol isNewsfeedUIVisible] for the UI class.
 */
+ (BOOL)isNewsfeedUIVisible;

/*!
 @abstract
 Display the Newsfeed UI.
 
 @discussion Calls [CBNewsfeedUIProtocol displayNewsfeed] for the UI class.
 */
+ (void)showNewsfeedUI;

/*!
 @abstract
 Close the Newsfeed UI.
 
 @discussion Calls [CBNewsfeedUIProtocol dismissNewsfeed] for the UI class.
 */
+ (void)closeNewsfeedUI;

/*!
 @abstract
 Check if the NotificationUI is visible.
 
 @return YES if the UI is visible, NO if not.
 
 @discussion Calls [CBNewsfeedUIProtocol isNofiticationUIVisible] for the UI class.
 */
+ (BOOL)isNotificationUIVisible;

/*!
 @abstract
 Display the Notification UI for the most recent CBStory.
 
 @discussion Calls [CBNewsfeedUIProtocol displayNotification] for the UI class.
 */
+ (void)showNotificationUI;

/*!
 @abstract
 Display the Notification UI for a specific CBStory.
 
 @param story CBStory object to display Notification UI for.
 
 @discussion Calls [CBNewsfeedUIProtocol displayNotification:] for the UI class.
 */
+ (void)showNotificationUIForStory:(CBStory *)story;

/*!
 @abstract
 Close the Notification UI.
 
 @discussion Calls [CBNewsfeedUIProtocol dismissNotification] for the UI class.
 */
+ (void)closeNotificationUI;

#pragma mark - CBNewsfeed Advanced Configuration

/*!
 @abstract
 Override how often the CBNewsfeed should attempt to get new data from the Chartboost API server.
 
 @param fetchTime Time in seconds to poll for messages.
 
 @discussion Default is 60 seconds; set to 0 to disable background fetch.

 Cannot be set to less than 60 seconds unless disabling.  If set to
 less than 60 seconds will use default fetch time of 60 seconds.
 */
+ (void)setFetchTime:(NSUInteger)fetchTime;

/*!
 @abstract
 Set a custom UI that implements the CBNewsfeedUIProtocol to replace the default Newsfeed UI.
 
 @param uiClass A Class reference that implements CBNewsfeedUIProtocol.
 
 @discussion Use this if you have created your own Newsfeed UI that conforms to the CBNewsfeedUIProtocol
 protocol.  If you want to use the default UI provided by Chartboost you do not need to use this method.
 */
+ (void)setUIClass:(Class)uiClass;

/*!
 @abstract
 Decide if Chartboost SDK should block for an age gate.
 
 @param shouldPause YES if Chartboost should pause for an age gate, NO otherwise.
 
 @discussion Set to control if Chartboost SDK should block for an age gate.
 
 Default is NO.
 */
+ (void)setShouldPauseStoryClickForConfirmation:(BOOL)shouldPause;

/*!
 @abstract
 Confirm if an age gate passed or failed. When specified Chartboost will wait for
 this call before showing the IOS App Store or navigating to a URL.
 
 @param pass The result of successfully passing the age confirmation.
 
 @discussion If you have configured your Chartboost experience to use the age gate feature
 then this method must be executed after the user has confirmed their age.  The Chartboost SDK
 will halt until this is done.
 */
+ (void)didPassAgeGate:(BOOL)pass;

@end

/*!
 @protocol CBNewsfeedDelegate
 
 @abstract
 Provide methods and callbacks to receive notifications of when the CBNewsfeed
 has taken specific actions or to more finely control the CBNewsfeed.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 
 All of the delegate methods are optional.
 */
@protocol CBNewsfeedDelegate <NSObject>

@optional

/*!
 @abstract
 Called when a user clicks a message in the newsfeed UI.
 
 @param message The CBStory object that the user interacted with.
 
 @discussion Implement to be notified of when a CBStory object has been clicked. 
 */
- (void)didClickMessage:(CBStory *)message;

/*!
 @abstract
 Called when a message expires due to a timer.
 
 @param message The CBStory object that expired.
 
 @discussion Implement to be notified of when a CBStory object has expired.
 */
- (void)didExpireMessage:(CBStory *)message;

/*!
 @abstract
 Called when a user views a message in the newsfeed UI.
 
 @param message The CBStory object that was viewed.
 
 @discussion Implement to be notified of when a CBStory object has been viewed.
 */
- (void)didViewMessage:(CBStory *)message;

/*!
 @abstract
 Called when a user clicks a notification.
 
 @param message The CBStory object that was clicked via the NotificationUI.
 
 @discussion Implement to be notified of when a CBStory object has been clicked via the NotificationUI.
 */
- (void)didClickNotification:(CBStory *)message;

/*!
 @abstract
 Called when a user viewes a notification.
 
 @param message The CBStory object that was viewed via the NotificationUI.
 
 @discussion Implement to be notified of when a CBStory object has been viewed via the NotificationUI.
 */
- (void)didViewNotification:(CBStory *)message;

/*!
 @abstract
 Called when new messages were retrieved from the server.
 
 @param messages An NSArray of new CBStory objects.
 
 @discussion Implement to be notified of when the Newsfeed has retrieved new CBStory messages from the
 Chartboost API servers.
 */
- (void)didGetNewMessages:(NSArray *)messages;

/*!
 @abstract
 Called when retrieveMessages successfully completes.
 
 @discussion Implement to be notified of when the Newsfeed has successfully completed a call to retrieve messages
 from the Chartboost API servers.  This does not indicate any changes have been made.  It is left to the
 developer to use the data methods located earlier in this file to check for the available CBStory objects
 to display.
 */
- (void)didRetrieveMessages;

/*!
 @abstract
 Called when retrieveMessages fails to complete.
 
 @param error The error response for the failure.
 
 @discussion Implement to be notified of when the Newsfeed has failed a call to retrieve messages
 from the Chartboost API servers.  This does not indicate any changes have been made.  It is left to the
 developer to use the data methods located earlier in this file to check for the available CBStory objects
 to display.
 */
- (void)didFailToRetrieveMessages:(NSError *)error;

/**
 * Implement to decide if the CBstory object should
 * automatically display a notification UI.
 *
 * @param message
 */

/*!
 @abstract
 Implement to decide if the CBStory object should automatically display a notification UI.
 
 @param message The CBStory object to display the notification UI for.
 
 @return YES if the notification UI should display, NO otherwise.
 
 @discussion Implement to decide if the CBstory object should automatically display a notification UI.
 
 Defaults to YES.
 */
- (BOOL)shouldAutomaticallyDisplayNotificationUI:(CBStory *)message;

/*!
 @abstract
 Called if Chartboost SDK pauses click actions awaiting confirmation from the user.
 
 @param message The CBStory object that the user interacted with.
 
 @discussion Use this method to display any gating you would like to prompt the user for input.
 Once confirmed call didPassAgeGate:(BOOL)pass to continue execution.
 */
- (void)didPauseStoryClickForConfirmation:(CBStory *)message;

@end
