//  LocalyticsSession.h
//  Copyright (C) 2013 Char Software Inc., DBA Localytics
//
//  This code is provided under the Localytics Modified BSD License.
//  A copy of this license has been distributed in a file called LICENSE
//  with this source code.
//
// Please visit www.localytics.com for more information.

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

#define CLIENT_VERSION              @"2.60.0"
#define MARKETING_PLATFORM

/*!
 @class LocalyticsSession
 @discussion The class which manages creating, collecting, & uploading a Localytics session.
 Please see the following guides for information on how to best use this
 library, sample code, and other useful information:
 <ul>
 <li><a href="http://wiki.localytics.com/index.php?title=Developer's_Integration_Guide">
 Main Developer's Integration Guide</a></li>
 </ul>

 <strong>Best Practices</strong>
 <ul>
 <li>Instantiate the LocalyticsSession object in applicationDidFinishLaunching.</li>
 <li>Open your session and begin your uploads in applicationDidFinishLaunching. This way the
 upload has time to complete and it all happens before your users have a
 chance to begin any data intensive actions of their own.</li>
 <li>Close the session in applicationWillTerminate, and in applicationDidEnterBackground.</li>
 <li>Resume the session in applicationWillEnterForeground.</li>
 <li>Do not call any Localytics functions inside a loop.  Instead, calls
 such as <code>tagEvent</code> should follow user actions.  This limits the
 amount of data which is stored and uploaded.</li>
 <li>Do not use multiple LocalticsSession objects to upload data with
 multiple application keys.  This can cause invalid state.</li>
 </ul>

 @author Localytics
 */

// Forward declaration
@protocol LocalyticsSessionDelegate;

@interface LocalyticsSession : NSObject

//************************************************************************************************//
#pragma mark - Singleton Accessors
/*!
 @method shared
 @abstract Accesses the Session object.  This is a Singleton class which maintains
 a single session throughout your application.  It is possible to manage your own
 session, but this is the easiest way to access the Localytics object throughout your code.
 The class is accessed within the code using the following syntax:
 [[LocalyticsSession shared] functionHere]
 So, to tag an event, all that is necessary, anywhere in the code is:
 [[LocalyticsSession shared] tagEvent:@"MY_EVENT"];
 */
+ (LocalyticsSession *)sharedLocalyticsSession;
+ (LocalyticsSession *)shared;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Simple Integration Methods
/*!
 @method integrateLocalytics
 @abstract Initializes the Localytics Object and auto-integrates basic open/close
 session and upload code.
 @param applicationKey The key unique for each application generated at www.localytics.com
 @param launchOptions Launch options
 */
- (void)integrateLocalytics:(NSString *)appKey launchOptions:(NSDictionary *)launchOptions;

/*!
 @method integratePushNotifications
 @abstract Lets the Localytics SDK handle registering for and handling push notifications
 @param remoteNotificationType The types of notifications for which to register
 */
- (void)integratePushNotifications:(UIRemoteNotificationType)remoteNotificationType;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Developer Options
/*!
 @property enableHTTPS
 @abstract (Optional) Determines whether or not HTTPS is used when calling the Localytics
 post URL. The default is NO.
 */
@property (nonatomic, assign) BOOL enableHTTPS;

/*!
 @property loggingEnabled
 @abstract (Optional) Determines whether or Localytics debugging information is shown
 to the console. The default is NO
 */
@property (nonatomic, assign) BOOL loggingEnabled;

/*!
 @property advertisingIdentifierEnabled
 @abstract (Optional) Determines whether or not IDFA is collected and sent to Localytics.
 The default is YES.
 */
@property (nonatomic, assign) BOOL advertisingIdentifierEnabled;

/*!
 @property sessionTimeoutInterval
 @abstrac (Optional) If an App stays in the background for more than this many seconds,
 start a new session when it returns to foreground.
 */
@property (atomic) float sessionTimeoutInterval;

/*!
 @property localyticsDelegate
 @abstract (Optional) Assign this delegate to the class you'd like to register to recieve
 the Localytics delegate callbacks (Defined at the end of this file)
 */
@property (nonatomic, assign) id<LocalyticsSessionDelegate> localyticsDelegate;

/*!
 @method setOptIn
 @abstract (OPTIONAL) Allows the application to control whether or not it will collect user data.
 Even if this call is used, it is necessary to continue calling upload().  No new data will be
 collected, so nothing new will be uploaded but it is necessary to upload an event telling the
 server this user has opted out.
 @param optedIn True if the user is opted in, false otherwise.
 */
- (void)setOptIn:(BOOL)optedIn;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Tag Events Methods
/*!
 @method tagEvent
 @abstract Allows a session to tag a particular event as having occurred.  For
 example, if a view has three buttons, it might make sense to tag
 each button click with the name of the button which was clicked.
 For another example, in a game with many levels it might be valuable
 to create a new tag every time the user gets to a new level in order
 to determine how far the average user is progressing in the game.
 <br>
 <strong>Tagging Best Practices</strong>
 <ul>
 <li>DO NOT use tags to record personally identifiable information.</li>
 <li>The best way to use tags is to create all the tag strings as predefined
 constants and only use those.  This is more efficient and removes the risk of
 collecting personal information.</li>
 <li>Do not set tags inside loops or any other place which gets called
 frequently.  This can cause a lot of data to be stored and uploaded.</li>
 </ul>
 <br>
 See the tagging guide at: http://wiki.localytics.com/
 @param event The name of the event which occurred.
 @param attributes (Optional) An object/hash/dictionary of key-value pairs, contains
 contextual data specific to the event.
 @param rerportAttributes (Optional) Additional attributes used for custom reporting.
 Available to Enterprise customers, please contact services for more details.
 @param customerValueIncrease (Optional) Numeric value, added to customer lifetime value.
 Integer expected. Try to use lowest possible unit, such as cents for US currency.
 */
- (void)tagEvent:(NSString *)event;

- (void)tagEvent:(NSString *)event
      attributes:(NSDictionary *)attributes;

- (void)tagEvent:(NSString *)event
      attributes:(NSDictionary *)attributes
customerValueIncrease:(NSNumber *)customerValueIncrease;

- (void)tagEvent:(NSString *)event
      attributes:(NSDictionary *)attributes
reportAttributes:(NSDictionary *)reportAttributes;

- (void)tagEvent:(NSString *)event
      attributes:(NSDictionary *)attributes
reportAttributes:(NSDictionary *)reportAttributes
customerValueIncrease:(NSNumber *)customerValueIncrease;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Custom Dimensions Methods
/*!
 @method setCustomDimension
 @abstract Sets the value of a custom dimension. Custom dimensions are dimensions
 which contain user defined data unlike the predefined dimensions such as carrier, model, and country.
 Once a value for a custom dimension is set, the device it was set on will continue to upload that value
 until the value is changed. To clear a value pass nil as the value.
 The proper use of custom dimensions involves defining a dimension with less than ten distinct possible
 values and assigning it to one of the four available custom dimensions. Once assigned this definition should
 never be changed without changing the App Key otherwise old installs of the application will pollute new data.
 */
- (void)setCustomDimension:(int)dimension value:(NSString *)value;

/*!
 @method customDimension
 @abstract Gets the custom dimension value for a given dimension. Avoid calling this on the main thread, as it
 may take some time for all pending database execution. */
- (NSString *)customDimension:(int)dimension;

/*!
 @method setValueForIdentifier
 @abstract Sets the value of a custom identifier. Identifiers are a form of key/value storage
 which contain custom user data. Identifiers might include things like email addresses, customer IDs, twitter
 handles, and facebook IDs.
 Once a value is set, the device it was set on will continue to upload that value until the value is changed.
 To delete a property, pass in nil as the value.
 */
- (void)setValueForIdentifier:(NSString *)identifierName value:(NSString *)value;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Tag Screen Method
/*!
 @method tagScreen
 @abstract Allows tagging the flow of screens encountered during the session.
 @param screen The name of the screen
 */
- (void)tagScreen:(NSString *)screen;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Customer Methods
/*!
 @method setCustomerName
 @abstract Record the customer name
 Once this value is set, the device it was set on will continue to upload that value until the value is changed.
 To delete the value, pass in nil.
 */
- (void)setCustomerName:(NSString *)email;

/*!
 @method setCustomerId
 @abstract Record your custom customer identifier
 Once this value is set, the device it was set on will continue to upload that value until the value is changed.
 To delete the value, pass in nil.
 */
- (void)setCustomerId:(NSString *)customerId;

/*!
 @method setCustomerId
 @abstract Record the customer's email address
 Once this value is set, the device it was set on will continue to upload that value until the value is changed.
 To delete the value, pass in nil.
 */
- (void)setCustomerEmail:(NSString *)email;
//************************************************************************************************//


//************************************************************************************************//
#pragma mark - Set Location Method
/*!
 @method setLocation
 @abstract Stores the user's location.  This will be used in all event and session calls.
 If your application has already collected the user's location, it may be passed to Localytics
 via this function.  This will cause all events and the session close to include the location
 information.  It is not required that you call this function.
 @param deviceLocation The user's location.
 */
- (void)setLocation:(CLLocationCoordinate2D)deviceLocation;
//************************************************************************************************//

//************************************************************************************************//
#pragma mark - Profile Methods
/*!
 @method setProfileValue:forAttribute:
 @abstract Sets the value of a profile attribute.
 @param value The value to set the profile attribute to. value can be one of the following: NSString,
 NSNumber(long & int), NSDate, NSArray of Strings, NSArray of NSNumbers(long & int), NSArray of Date,
 nil. Passing in a 'nil' value will result in that attribute being deleted from the profile
 @param attribute The attribute is the name of the profile attribute
 */
- (void)setProfileValue:(NSObject<NSCopying> *)value forAttribute:(NSString *)attribute;
//************************************************************************************************//

//************************************************************************************************//
#pragma mark - Advanced Integration Methods
/*!
 @method LocalyticsSession
 @abstract Initializes the Localytics Object.  Not necessary if you choose to use startSession.
 @param applicationKey The key unique for each application generated at www.localytics.com
 */
- (void)LocalyticsSession:(NSString *)appKey;

/*!
 @method startSession
 @abstract An optional convenience initialize method that also calls the LocalyticsSession, open &
 upload methods. Best Practice is to call open & upload immediately after Localytics Session when loading an app,
 this method fascilitates that behavior.
 It is recommended that this call be placed in <code>applicationDidFinishLaunching</code>.
 @param applicationKey The key unique for each application generated
 at www.localytics.com
 */
- (void)startSession:(NSString *)appKey;

/*!
 @method open
 @abstract Opens the Localytics session. Not necessary if you choose to use startSession.
 The session time as presented on the website is the time between <code>open</code> and the
 final <code>close</code> so it is recommended to open the session as early as possible, and close
 it at the last moment.  The session must be opened before any tags can
 be written.  It is recommended that this call be placed in <code>applicationDidFinishLaunching</code>.
 <br>
 If for any reason this is called more than once every subsequent open call
 will be ignored.
 */
- (void)open;

/*!
 @method resume
 @abstract Resumes the Localytics session.  When the App enters the background, the session is
 closed and the time of closing is recorded.  When the app returns to the foreground, the session
 is resumed.  If the time since closing is greater than BACKGROUND_SESSION_TIMEOUT, (15 seconds
 by default) a new session is created, and uploading is triggered.  Otherwise, the previous session
 is reopened.
 */
- (void)resume;

/*!
 @method close
 @abstract Closes the Localytics session.  This should be called in
 <code>applicationWillTerminate</code>.
 <br>
 If close is not called, the session will still be uploaded but no
 events will be processed and the session time will not appear. This is
 because the session is not yet closed so it should not be used in
 comparison with sessions which are closed.
 */
- (void)close;

/*!
 @method upload
 @abstract Creates a low priority thread which uploads any Localytics data already stored
 on the device.  This should be done early in the process life in order to
 guarantee as much time as possible for slow connections to complete.  It is also reasonable
 to upload again when the application is exiting because if the upload is cancelled the data
 will just get uploaded the next time the app comes up.
 */
- (void)upload;

/*!
 @method setPushToken
 @abstract Stores the device's APNS token. This will be used in all event and session calls.
 @param pushToken device token returned by application:didRegisterForRemoteNotificationsWithDeviceToken:
 */
- (void)setPushToken:(NSData *)pushToken;

#ifdef MARKETING_PLATFORM
/*!
 @method handleRemoteNotification
 @abstract Used to record performance data for push notifications
 @param notificationInfo The dictionary from either didFinishLaunchingWithOptions
 or didReceiveRemoteNotification should be passed on to this method
 */
- (void)handleRemoteNotification:(NSDictionary *)notificationInfo;
#endif

@end

#pragma mark -
@protocol LocalyticsSessionDelegate <NSObject>
@optional

/*!
 @method localyticsWillResumeSession
 @abstract Register for this callback to be notified when Localytics will resume
 a session. See the on the 'resume' method for additional details.
 @param willResumeExistingSession This flag will indicate if Localytics restored an existing
 session or started a new one.
 */
- (void)localyticsWillResumeSession:(BOOL)willResumeExistingSession;

/*!
 @method localyticsDidResumeSession
 @abstract Register for this callback to be notified when Localytics has resumed
 a session. See the on the 'resume' method for additional details.
 @param didResumeExistingSession This flag will indicate if Localytics restored an existing
 session or started a new one.
 */
- (void)localyticsDidResumeSession:(BOOL)didResumeExistingSession;

/*!
 @method localyticsResumedSession
 @abstract Register for this callback to be notified when Localytics has either
 resumed a previous session or created a new one. See the on the 'resume' method
 for additional details.
 @param didResumeExistingSession This flag will indicate if Localytics restored an existing
 session or started a new one.
 @deprecated This method is deprecated. Use 'localyticsDidResumeSession:' instead.
 */
- (void)localyticsResumedSession:(BOOL)didResumeExistingSession __attribute__((deprecated("This method is deprecated. Use 'localyticsDidResumeSession:' instead.")));

/*!
 @method localyticsPrepareUploadBody
 @abstract Register for this callback if you wish to modify the upload body before
 being sent to Localytics
 @param uploadBody The upload body as prepared by Localytics
 @return The modified upload body
 */
- (NSString *)localyticsPrepareUploadBody:(NSString *)uploadBody;

@end
