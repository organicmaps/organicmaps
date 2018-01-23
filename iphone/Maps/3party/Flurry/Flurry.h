//
//  Flurry.h
//  Flurry iOS Analytics Agent
//
//  Copyright 2016 Flurry, Inc. All rights reserved.
//	
//	Methods in this header file are for use with Flurry Analytics

#import <UIKit/UIKit.h>
#if !TARGET_OS_WATCH
#import <StoreKit/StoreKit.h>
#endif
#if TARGET_OS_TV
@class JSContext;
#endif

#import "FlurrySessionBuilder.h"

typedef enum {
    FlurryEventFailed = 0,
    FlurryEventRecorded,
    FlurryEventUniqueCountExceeded,
    FlurryEventParamsCountExceeded,
    FlurryEventLogCountExceeded,
    FlurryEventLoggingDelayed,
    FlurryEventAnalyticsDisabled
} FlurryEventRecordStatus;


/*!
 *  @brief Enum for logging events that occur within a syndicated app
 *  @since 6.7.0
 *
 */

typedef enum {
    FlurrySyndicationReblog      = 0,
    FlurrySyndicationFastReblog  = 1,
    FlurrySyndicationSourceClick = 2,
    FlurrySyndicationLike        = 3,
    FlurrySyndicationShareClick  = 4,
    FlurrySyndicationPostSend    = 5
    
}FlurrySyndicationEvent;

extern NSString* const kSyndicationiOSDeepLink;
extern NSString* const kSyndicationAndroidDeepLink;
extern NSString* const kSyndicationWebDeepLink;


typedef enum {
    FlurryTransactionRecordFailed = 0,
    FlurryTransactionRecorded,
    FlurryTransactionRecordExceeded,
    FlurryTransactionRecodingDisabled
} FlurryTransactionRecordStatus;


/*!
 *  @brief Provides all available delegates for receiving callbacks related to Flurry analytics.
 *
 *  Set of methods that allow developers to manage and take actions within
 *  different phases of App.
 *
 *  @note This class serves as a delegate for Flurry. \n
 *  For additional information on how to use Flurry's Ads SDK to
 *  attract high-quality users and monetize your user base see <a href="http://wiki.flurry.com/index.php?title=Publisher">Support Center - Publisher</a>.
 *  @author 2010 - 2014 Flurry, Inc. All Rights Reserved.
 *  @version 6.3.0
 *
 */
@protocol FlurryDelegate <NSObject>

/*!
 *  @brief Invoked when analytics session is created
 *  @since 6.3.0
 *
 *  This method informs the app that an analytics session is created.
 *
 *  @see Flurry#startSession for details on session.
 *
 *  @param info A dictionary of session information: sessionID, apiKey
 */
- (void)flurrySessionDidCreateWithInfo:(NSDictionary*)info;

@end

/*!
 *  @brief Provides all available methods for defining and reporting Analytics from use
 *  of your app.
 * 
 *  Set of methods that allow developers to capture detailed, aggregate information
 *  regarding the use of their app by end users.
 *  
 *  @note This class provides methods necessary for correct function of Flurry.h.
 *  For information on how to use Flurry's Ads SDK to
 *  attract high-quality users and monetize your user base see <a href="https://developer.yahoo.com/flurry/docs/howtos">Support Center - Publishers</a>.
 *
 *  @version 4.3.0
 * 
 */

@interface Flurry : NSObject {
}

/** @name Pre-Session Calls
 *  Optional sdk settings that should be called before start session. 
 */
//@{

/*!
 *  @brief Explicitly specifies the App Version that Flurry will use to group Analytics data.
 *  @since 2.7
 *
 *  @deprecated since 7.7.0, please use FlurrySessionBuilder in place of calling this API.
 *  This method will be removed in a future version of the SDK.
 *
 *  This is an optional method that overrides the App Version Flurry uses for reporting. Flurry will
 *  use the CFBundleVersion in your info.plist file when this method is not invoked.
 *
 *  @note There is a maximum of 605 versions allowed for a single app. \n
 *  This method must be called prior to invoking #startSession:.
 *
 *  @param version The custom version name.
 */
+ (void)setAppVersion:(NSString*) version __attribute__ ((deprecated));


#if TARGET_OS_TV
/*!
 *  @brief Sets the minimum number of events before a partial session report is sent to Flurry.
 *  @since 1.0.0
 *
 *  @deprecated since 7.7.0, please use FlurrySessionBuilder in place of calling this API.
 *  This method will be removed in a future version of the SDK.
 *
 *  This is an optional method that sets the minimum number of events before a partial session report is sent to Flurry.
 *  The acceptable values are between 5 and 50.
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param  count The number of events after which partial session report is sent to Flurry.
 */
+ (void)setTVEventFlushCount:(short)count __attribute__ ((deprecated));

/*!
 *  @brief Sets the minimum duration (in minutes) before a partial session report is sent to Flurry.
 *  @since 1.0.0
 *
 *  @deprecated since 7.7.0, please use FlurrySessionBuilder in place of calling this API.
 *  This method will be removed in a future version of the SDK.
 *
 *  This is an optional method that sets the minimum duration (in minutes) before a partial session report is sent to Flurry.
 *  The acceptable values are between 5 and 60 minutes.
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param duration The period after which a partial session report is sent to Flurry.
 */
+ (void)setTVSessionReportingInterval:(short)duration __attribute__ ((deprecated));
#endif

/*!
 *  @brief Retrieves the Flurry Agent Build Version.
 *  @since 2.7
 *
 *  This is an optional method that retrieves the Flurry Agent Version the app is running under. 
 *  It is most often used if reporting an unexpected behavior of the SDK to <a href="mailto:iphonesupport@flurry.com">
 *  Flurry Support</a>
 *
 *  @see #setLogLevel: for information on how to view debugging information on your console.
 *
 *  @return The agent version of the Flurry SDK.
 *
 */
+ (NSString *)getFlurryAgentVersion;

/*!
 *  @brief Displays an exception in the debug log if thrown during a Session.
 *  @since 2.7
 *
 *  This is an optional method that augments the debug logs with exceptions that occur during the session.
 *  You must both capture exceptions to Flurry and set debug logging to enabled for this method to
 *  display information to the console. The default setting for this method is @c NO.
 *
 *  @note This method can be called at any point in the execution of your application and
 *  the setting will take effect for SDK activity after this call.
 *
 *  @see #setLogLevel: for information on how to view debugging information on your console. \n
 *  #logError:message:exception: for details on logging exceptions. \n
 *  #logError:message:error: for details on logging errors.
 *
 *  @param value @c YES to show errors in debug logs, @c NO to omit errors in debug logs.
 */
+ (void)setShowErrorInLogEnabled:(BOOL)value;

/*!
 *  @brief Generates debug logs to console.
 *  @since 2.7
 *
 *  @deprecated since 7.7.0, please use setLogLevel or FlurrySessionBuilder in place of calling this API.
 *  This method will be removed in a future version of the SDK.
 *
 *  This is an optional method that displays debug information related to the Flurry SDK.
 *  display information to the console. The default setting for this method is @c NO 
 *  which sets the log level to @c FlurryLogLevelCriticalOnly.
 *  When set to @c YES the debug log level is set to @c FlurryLogLevelDebug
 *
 *  @param value @c YES to show debug logs, @c NO to omit debug logs.
 *
 */
+ (void)setDebugLogEnabled:(BOOL)value __attribute__ ((deprecated));

/*!
 *  @brief Generates debug logs to console.
 *  @since 4.2.2
 *
 *  This is an optional method that displays debug information related to the Flurry SDK.
 *  display information to the console. The default setting for this method is @c FlurryLogLevelCriticalOnly.
 *
 *  @note The log level can be changed at any point in the execution of your application and 
 *  the level that is set will take effect for SDK activity after this call.
 *
 *  @param value Log level
 *
 */
+ (void)setLogLevel:(FlurryLogLevel)value;

/*!
 *  @brief Set the timeout for expiring a Flurry session.
 *  @since 2.7
 *
 *  @deprecated since 7.7.0, please use FlurrySessionBuilder in place of calling this API.
 *  This method will be removed in a future version of the SDK.
 * 
 *  This is an optional method that sets the time the app may be in the background before 
 *  starting a new session upon resume.  The default value for the session timeout is 10 
 *  seconds in the background.
 * 
 *  @note This method must be called prior to invoking #startSession:.
 * 
 *  @param seconds The time in seconds to set the session timeout to.
 */
+ (void)setSessionContinueSeconds:(int)seconds __attribute__ ((deprecated));


#if !TARGET_OS_TV
/*!
 *  @brief Enable automatic collection of crash reports.
 *  @since 4.1
 *  @deprecated since 7.7.0, please use FlurrySessionBuilder in place of calling this API.
 *
 *  This is an optional method that collects crash reports when enabled. The
 *  default value is @c NO.
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param value @c YES to enable collection of crash reports.
 */
+ (void)setCrashReportingEnabled:(BOOL)value __attribute__ ((deprecated));
#endif

//@}

/*!
 *  @brief Start a Flurry session for the project denoted by @c apiKey.
 *  @since 2.6
 * 
 *  This method serves as the entry point to Flurry Analytics collection.  It must be
 *  called in the scope of @c applicationDidFinishLaunching.  The session will continue 
 *  for the period the app is in the foreground until your app is backgrounded for the 
 *  time specified in #setSessionContinueSeconds:. If the app is resumed in that period
 *  the session will continue, otherwise a new session will begin.
 *
 *  Crash reporting will not be enabled. See #setCrashReportingEnabled: for
 *  more information.
 * 
 *  @note If testing on a simulator, please be sure to send App to background via home
 *  button. Flurry depends on the iOS lifecycle to be complete for full reporting.
 * 
 * @see #setSessionContinueSeconds: for details on setting a custom session timeout.
 *
 * @code
 *  - (void)applicationDidFinishLaunching:(UIApplication *)application 
 {
 // Optional Flurry startup methods
 [Flurry startSession:@"YOUR_API_KEY"];
 // ....
 }
 * @endcode
 * 
 * @param apiKey The API key for this project.
 */

+ (void)startSession:(NSString *)apiKey;


/*!
 *  @brief Start a Flurry session for the project denoted by @c apiKey.
 *  @since 4.0.8
 *
 *  This method serves as the entry point to Flurry Analytics collection.  It must be
 *  called in the scope of @c applicationDidFinishLaunching passing in the launchOptions param.
 *  The session will continue
 *  for the period the app is in the foreground until your app is backgrounded for the
 *  time specified in #setSessionContinueSeconds:. If the app is resumed in that period
 *  the session will continue, otherwise a new session will begin.
 *
 *  @note If testing on a simulator, please be sure to send App to background via home
 *  button. Flurry depends on the iOS lifecycle to be complete for full reporting.
 *
 * @see #setSessionContinueSeconds: for details on setting a custom session timeout.
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
 {
 // Optional Flurry startup methods
 [Flurry startSession:@"YOUR_API_KEY" withOptions:launchOptions];
 // ....
 }
 * @endcode
 *
 * @param apiKey The API key for this project.
 * @param options passed launchOptions from the applicatin's didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
 
 */
+ (void) startSession:(NSString *)apiKey withOptions:(id)options;


/*!
 *  @brief Start a Flurry session for the project denoted by @c apiKey.
 *  @since 7.7.0
 *
 *  This method serves as the entry point to Flurry Analytics collection.  It must be
 *  called in the scope of @c applicationDidFinishLaunching passing in the launchOptions param.
 *  The session will continue
 *  for the period the app is in the foreground until your app is backgrounded for the
 *  time specified in #setSessionContinueSeconds:. If the app is resumed in that period
 *  the session will continue, otherwise a new session will begin.
 *
 *  @note If testing on a simulator, please be sure to send App to background via home
 *  button. Flurry depends on the iOS lifecycle to be complete for full reporting.
 *
 * @see #setSessionContinueSeconds: for details on setting a custom session timeout.
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
    {
        // Optional Flurry startup methods
        FlurrySessionBuilder* builder = [[[[[FlurrySessionBuilder new] withLogLevel:FlurryLogLevelDebug]
                                                                 withCrashReporting:NO]
                                                         withSessionContinueSeconds:10]
                                                                     withAppVersion:@"0.1.2"];
 
        [Flurry startSession:@"YOUR_API_KEY" withOptions:launchOptions withSessionBuilder:sessionBuilder];
        // ....
    }
 * @endcode
 *
 * @param apiKey The API key for this project.
 * @param options passed launchOptions from the applicatin's didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
 * @param sessionBuilder pass in the session builder object to specify that session construction options
 
 */
+ (void) startSession:(NSString *)apiKey withOptions:(id)options withSessionBuilder:(FlurrySessionBuilder*) sessionBuilder;


/*!
 *  @brief Start a Flurry session for the project denoted by @c apiKey.
 *  @since 7.7.0
 *
 *  This method serves as the entry point to Flurry Analytics collection.  It must be
 *  called in the scope of @c applicationDidFinishLaunching passing in the launchOptions param.
 *  The session will continue
 *  for the period the app is in the foreground until your app is backgrounded for the
 *  time specified in #setSessionContinueSeconds:. If the app is resumed in that period
 *  the session will continue, otherwise a new session will begin.
 *
 *  @note If testing on a simulator, please be sure to send App to background via home
 *  button. Flurry depends on the iOS lifecycle to be complete for full reporting.
 *
 * @see #setSessionContinueSeconds: for details on setting a custom session timeout.
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
    {
        // Optional Flurry startup methods
        FlurrySessionBuilder* builder = [[[[[FlurrySessionBuilder new] withLogLevel:FlurryLogLevelDebug]
                                                                 withCrashReporting:NO]
                                                         withSessionContinueSeconds:10]
                                                                     withAppVersion:@"0.1.2"];
 
        [Flurry startSession:@"YOUR_API_KEY" withSessionBuilder:sessionBuilder];
        // ....
    }
 * @endcode
 *
 * @param apiKey The API key for this project.
 * @param sessionBuilder pass in the session builder object to specify that session construction options
 */
+ (void) startSession:(NSString *)apiKey withSessionBuilder:(FlurrySessionBuilder *)sessionBuilder;

/*!
 *  @brief Returns true if a session currently exists and is active.
 *  @since 6.0.0
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
    {
        // Optional Flurry startup methods
        [Flurry activeSessionExists];
        // ....
    }
 * @endcode
 *
 */
+ (BOOL)activeSessionExists;

/*!
 *  @brief Returns the session ID of the current active session.
 *  @since 6.3.0
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
    {
        // Optional Flurry startup methods
        [Flurry getSessionID];
        // ....
    }
 * @endcode
 *
 
 */
+ (NSString*)getSessionID;


/*!
 *  @brief Set Flurry delegate for callback on session creation.
 *  @since 6.3.0
 *
 * @code
 *  - (BOOL) application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
    {
        // Optional Flurry startup methods
        // If self implements protocol, FlurryDelegate
        [Flurry setDelegate:self];
        // ....
    }
 * @endcode
 *
 
 */
+ (void)setDelegate:(id<FlurryDelegate>)delegate;


#if !TARGET_OS_TV
/*!
 *  @brief Pauses a Flurry session left running in background.
 *  @since 4.2.2
 *
 *  This method should be used in case of #setBackgroundSessionEnabled: set to YES. It can be
 *  called when application finished all background tasks (such as playing music) to pause session.
 *
 * @see #setBackgroundSessionEnabled: for details on setting a custom behaviour on resigning activity.
 *
 * @code
 *  - (void)allBackgroundTasksFinished
    {
        // ....
        [Flurry pauseBackgroundSession];
        // ....
    }
 * @endcode
 *
 */
+ (void)pauseBackgroundSession;
#endif

/*!
 *  @brief Adds an session origin and deep link attached to each session specified by @c sessionOriginName and  @c deepLink.
 *  @since 6.5.0
 *
 *  This method allows you to specify session origin and deep link for each session. This is different than addOrigin which is used for third party
 *  wrappers after every session start.
 *
 *
 *  @code
 *  - (void)interestingMethod
    {
        // ... after calling startSession
        [Flurry addSessionOrigin:@"facebuk"];
        // more code ...
    }
 *  @endcode
 *
 *  @param sessionOriginName    Name of the origin.
 *  @param deepLink             Url of the deep Link.
 */
+ (void)addSessionOrigin:(NSString *)sessionOriginName  withDeepLink:(NSString*)deepLink;

/*!
 *  @brief Adds an session origin attached to each session specified by @c sessionOriginName.
 *  @since 6.5.0
 *
 *  This method allows you to specify session origin for each session. This is different than addOrigin which is used for third party
 *  wrappers after every session start.
 *
 *
 *  @code
 *  - (void)interestingMethod
    {
        // ... after calling startSession
        [Flurry addSessionOrigin:@"facebuk"];
        // more code ...
    }
 *  @endcode
 *
 *  @param sessionOriginName    Name of the origin.
 */
+ (void)addSessionOrigin:(NSString *)sessionOriginName;

/*!
 *  @brief Adds a custom parameterized session parameters @c parameters.
 *  @since 6.5.0
 *
 *  This method allows you to associate parameters with an session. Parameters
 *  are valuable as they allow you to store characteristics of an session.
 *
 *  @note You should not pass private or confidential information about your origin info in a
 *  custom origin. \n
 *  A maximum of 20 parameter names may be associated with any origin. Sending
 *  over 20 parameter names with a single origin will result in no parameters being logged
 *  for that origin.
 *
 *
 *  @code

 *  @endcode
 *
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 */
+ (void)sessionProperties:(NSDictionary *)parameters;

/*!
 *  @brief Adds an SDK origin specified by @c originName and @c originVersion.
 *  @since 5.0.0
 *
 *  This method allows you to specify origin within your Flurry SDK wrapper. As a general rule
 *  you should capture all the origin info related to your wrapper for Flurry SDK after every session start.
 *
 *  @see #addOrigin:withVersion:withParameters: for details on reporting origin info with parameters. \n
 *
 *  @code
 *  - (void)interestingSDKWrapperLibraryfunction
 {
     // ... after calling startSession
     [Flurry addOrigin:@"Interesting_Wrapper" withVersion:@"1.0.0"];
     // more code ...
 }
 *  @endcode
 *
 *  @param originName    Name of the origin.
 *  @param originVersion Version string of the origin wrapper
 */
+ (void)addOrigin:(NSString *)originName withVersion:(NSString*)originVersion;

/*!
 *  @brief Adds a custom parameterized origin specified by @c originName with @c originVersion and @c parameters.
 *  @since 5.0.0
 *
 *  This method overloads #addOrigin to allow you to associate parameters with an origin attribute. Parameters
 *  are valuable as they allow you to store characteristics of an origin.
 *
 *  @note You should not pass private or confidential information about your origin info in a
 *  custom origin. \n
 *  A maximum of 9 parameter names may be associated with any origin. Sending
 *  over 10 parameter names with a single origin will result in no parameters being logged
 *  for that origin.
 *
 *
 *  @code
 *  - (void)userPurchasedSomethingCool
    {
        NSDictionary *params =
        [NSDictionary dictionaryWithObjectsAndKeys:@"Origin Info Item", // Parameter Value
            @"Origin Info Item Key", // Parameter Name
            nil];
        // ... after calling startSession
        [Flurry addOrigin:@"Interesting_Wrapper" withVersion:@"1.0.0"];
        // more code ...
    }
 *  @endcode
 *
 *  @param originName    Name of the origin.
 *  @param originVersion Version string of the origin wrapper
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 */
+ (void)addOrigin:(NSString *)originName withVersion:(NSString*)originVersion withParameters:(NSDictionary *)parameters;

/** @name Event and Error Logging
 *  Methods for reporting custom events and errors during the session. 
 */
//@{

/*!
 *  @brief Records a custom event specified by @c eventName.
 *  @since 2.8.4
 * 
 *  This method allows you to specify custom events within your app.  As a general rule
 *  you should capture events related to user navigation within your app, any action 
 *  around monetization, and other events as they are applicable to tracking progress
 *  towards your business goals. 
 * 
 *  @note You should not pass private or confidential information about your users in a
 *  custom event. \n
 *  Where applicable, you should make a concerted effort to use timed events with
 *  parameters (#logEvent:withParameters:timed:) or events with parameters 
 *  (#logEvent:withParameters:). This provides valuable information around the time the user
 *  spends within an action (e.g. - time spent on a level or viewing a page) or characteristics
 *  of an action (e.g. - Buy Event that has a Parameter of Widget with Value Golden Sword).
 * 
 *  @see #logEvent:withParameters: for details on storing events with parameters. \n
 *  #logEvent:timed: for details on storing timed events. \n
 *  #logEvent:withParameters:timed: for details on storing timed events with parameters. \n
 *  #endTimedEvent:withParameters: for details on stopping a timed event and (optionally) updating 
 *  parameters.
 *
 *  @code
 *  - (void)interestingAppAction 
    {
        [Flurry logEvent:@"Interesting_Action"];
        // Perform interesting action
    }
 *  @endcode
 * 
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logEvent:(NSString *)eventName;

/*!
 *  @brief Records a custom parameterized event specified by @c eventName with @c parameters.
 *  @since 2.8.4
 * 
 *  This method overloads #logEvent to allow you to associate parameters with an event. Parameters
 *  are extremely valuable as they allow you to store characteristics of an action. For example,
 *  if a user purchased an item it may be helpful to know what level that user was on.
 *  By setting this parameter you will be able to view a distribution of levels for the purcahsed
 *  event on the <a href="http://dev.flurry.com">Flurrly Dev Portal</a>.
 * 
 *  @note You should not pass private or confidential information about your users in a
 *  custom event. \n
 *  A maximum of 10 parameter names may be associated with any event. Sending
 *  over 10 parameter names with a single event will result in no parameters being logged
 *  for that event. You may specify an infinite number of Parameter values. For example,
 *  a Search Box would have 1 parameter name (e.g. - Search Box) and many values, which would
 *  allow you to see what values users look for the most in your app. \n
 *  Where applicable, you should make a concerted effort to use timed events with
 *  parameters (#logEvent:withParameters:timed:). This provides valuable information 
 *  around the time the user spends within an action (e.g. - time spent on a level or 
 *  viewing a page).
 * 
 *  @see #logEvent:withParameters:timed: for details on storing timed events with parameters. \n
 *  #endTimedEvent:withParameters: for details on stopping a timed event and (optionally) updating 
 *  parameters.
 *
 *  @code
 *  - (void)userPurchasedSomethingCool 
    {
        NSDictionary *params =
        [NSDictionary dictionaryWithObjectsAndKeys:@"Cool Item", // Parameter Value
            @"Item Purchased", // Parameter Name
            nil];
        [Flurry logEvent:@"Something Cool Purchased" withParameters:params];
        // Give user cool item
    }
 *  @endcode
 * 
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;

/*!
 *  @brief Records an app exception. Commonly used to catch unhandled exceptions.
 *  @since 2.7
 * 
 *  This method captures an exception for reporting to Flurry. We recommend adding an uncaught
 *  exception listener to capture any exceptions that occur during usage that is not
 *  anticipated by your app.
 * 
 *  @see #logError:message:error: for details on capturing errors.
 *
 *  @code
 *  - (void) uncaughtExceptionHandler(NSException *exception) 
    {
        [Flurry logError:@"Uncaught" message:@"Crash!" exception:exception];
    }
 
    - (void)applicationDidFinishLaunching:(UIApplication *)application
    {
        NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
        [Flurry startSession:@"YOUR_API_KEY"];
        // ....
    }
 *  @endcode
 * 
 *  @param errorID Name of the error.
 *  @param message The message to associate with the error.
 *  @param exception The exception object to report.
 */
+ (void)logError:(NSString *)errorID message:(NSString *)message exception:(NSException *)exception;

/*!
 *  @brief Records an app error.
 *  @since 2.7
 * 
 *  This method captures an error for reporting to Flurry.
 * 
 *  @see #logError:message:exception: for details on capturing exceptions.
 *
 *  @code
 *  - (void) webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error 
    {
        [Flurry logError:@"WebView No Load" message:[error localizedDescription] error:error];
    }
 *  @endcode
 * 
 *  @param errorID Name of the error.
 *  @param message The message to associate with the error.
 *  @param error The error object to report.
 */
+ (void)logError:(NSString *)errorID message:(NSString *)message error:(NSError *)error;

/*!
 *  @brief Records a timed event specified by @c eventName.
 *  @since 2.8.4
 * 
 *  This method overloads #logEvent to allow you to capture the length of an event. This can
 *  be extremely valuable to understand the level of engagement with a particular action. For
 *  example, you can capture how long a user spends on a level or reading an article.
 * 
 *  @note You should not pass private or confidential information about your users in a
 *  custom event. \n
 *  Where applicable, you should make a concerted effort to use parameters with your timed 
 *  events (#logEvent:withParameters:timed:). This provides valuable information 
 *  around the characteristics of an action (e.g. - Buy Event that has a Parameter of Widget with 
 *  Value Golden Sword).
 * 
 *  @see #logEvent:withParameters:timed: for details on storing timed events with parameters. \n
 *  #endTimedEvent:withParameters: for details on stopping a timed event and (optionally) updating 
 *  parameters.
 *
 *  @code
 *  - (void)startLevel 
    {
        [Flurry logEvent:@"Level Played" timed:YES];
        // Start user on level
    }
 
    - (void)endLevel
    {
        [Flurry endTimedEvent:@"Level Played" withParameters:nil];
        // User done with level
    }
 *  @endcode
 * 
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param timed Specifies the event will be timed..
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logEvent:(NSString *)eventName timed:(BOOL)timed;

/*!
 *  @brief Records a custom parameterized timed event specified by @c eventName with @c parameters.
 *  @since 2.8.4
 * 
 *  This method overloads #logEvent to allow you to capture the length of an event with parameters.
 *  This can be extremely valuable to understand the level of engagement with a particular action 
 *  and the characteristics associated with that action. For example, you can capture how long a user 
 *  spends on a level or reading an article. Parameters can be used to capture, for example, the
 *  author of an article or if something was purchased while on the level.
 * 
 *  @note You should not pass private or confidential information about your users in a
 *  custom event.
 *
 *  @see #endTimedEvent:withParameters: for details on stopping a timed event and (optionally) updating 
 *  parameters.
 *
 *  @code
 *  - (void)startLevel 
    {
        NSDictionary *params =
        [NSDictionary dictionaryWithObjectsAndKeys:@"100", // Parameter Value
            @"Current Points", // Parameter Name
            nil];
 
        [Flurry logEvent:@"Level Played" withParameters:params timed:YES];
        // Start user on level
    }
 
    - (void)endLevel
    {
        // User gained additional 100 points in Level
        NSDictionary *params =
            [NSDictionary dictionaryWithObjectsAndKeys:@"200", // Parameter Value
                @"Current Points", // Parameter Name
                nil];
        [Flurry endTimedEvent:@"Level Played" withParameters:params];
        // User done with level
    }
 *  @endcode
 * 
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 *  @param timed Specifies the event will be timed..
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters timed:(BOOL)timed;

/*!
 *  @brief Ends a timed event specified by @c eventName and optionally updates parameters with @c parameters.
 *  @since 2.8.4
 * 
 *  This method ends an existing timed event.  If parameters are provided, this will overwrite existing
 *  parameters with the same name or create new parameters if the name does not exist in the parameter
 *  map set by #logEvent:withParameters:timed:.
 * 
 *  @note You should not pass private or confidential information about your users in a
 *  custom event. \n
 *  If the app is backgrounded prior to ending a timed event, the Flurry SDK will automatically
 *  end the timer on the event. \n 
 *  #endTimedEvent:withParameters: is ignored if called on a previously
 *  terminated event.
 *
 *  @see #logEvent:withParameters:timed: for details on starting a timed event with parameters.
 *
 *  @code
 *  - (void)startLevel 
    {
        NSDictionary *params =
            [NSDictionary dictionaryWithObjectsAndKeys:@"100", // Parameter Value
            @"Current Points", // Parameter Name
        nil];
 
    [Flurry logEvent:@"Level Played" withParameters:params timed:YES];
    // Start user on level
    }
 
    - (void)endLevel
    {
        // User gained additional 100 points in Level
        NSDictionary *params =
            [NSDictionary dictionaryWithObjectsAndKeys:@"200", // Parameter Value
                @"Current Points", // Parameter Name
                nil];
        [Flurry endTimedEvent:@"Level Played" withParameters:params];
        // User done with level
    }
 *  @endcode
 * 
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 */
+ (void)endTimedEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;	// non-nil parameters will update the parameters

//@}


#if !TARGET_OS_TV
/** @name Page View Methods
 *  Count page views. 
 */
//@{

/*!
 *  @deprecated
 *  @brief see +(void)logAllPageViewsForTarget:(id)target; for details
 *  @since 2.7
 *  This method does the same as +(void)logAllPageViewsForTarget:(id)target method and is left for backward compatibility
 */
+ (void)logAllPageViews:(id)target __attribute__ ((deprecated));		
/*!
 *  @brief Automatically track page views on a @c UINavigationController or @c UITabBarController.
 *  @since 4.3
 * 
 *  This method increments the page view count for a session based on traversing a UINavigationController
 *  or UITabBarController. The page view count is only a counter for the number of transitions in your
 *  app. It does not associate a name with the page count. To associate a name with a count of occurences
 *  see #logEvent:.
 * 
 *  @note If you need to release passed target, you should call counterpart method + (void)stopLogPageViewsForTarget:(id)target before;
 *
 *  @see #logPageView for details on explictly incrementing page view count.
 *
 *  @code
 *  -(void) trackViewsFromTabBar:(UITabBarController*) tabBar
    {
        [Flurry logAllPageViewsForTarget:tabBar];
    }
 *  @endcode
 * 
 *  @param target The navigation or tab bar controller.
 */
+ (void)logAllPageViewsForTarget:(id)target;

/*!
 *  @brief Stops logging page views on previously observed with logAllPageViewsForTarget: @c UINavigationController or @c UITabBarController.
 *  @since 4.3
 * 
 *  Call this method before instance of @c UINavigationController or @c UITabBarController observed with logAllPageViewsForTarget: is released.
 *
 *  @code
 * -(void) dealloc
    {
        [Flurry stopLogPageViewsForTarget:_tabBarController];
        [_tabBarController release];
        [super dealloc];
    }
 *  @endcode
 * 
 *  @param target The navigation or tab bar controller.
 */
+ (void)stopLogPageViewsForTarget:(id)target;

/*!
 *  @brief Explicitly track a page view during a session.
 *  @since 2.7
 * 
 *  This method increments the page view count for a session when invoked. It does not associate a name
 *  with the page count. To associate a name with a count of occurences see #logEvent:.
 *
 *  @see #logAllPageViews for details on automatically incrementing page view count based on user
 *  traversing navigation or tab bar controller.
 *
 *  @code
 *  -(void) trackView 
    {
        [Flurry logPageView];
    }
 *  @endcode
 *
 */
+ (void)logPageView;

//@}
#endif



/** @name User Info
 *  Methods to set user information. 
 */
//@{

/*!
 *  @brief Assign a unique id for a user in your app.
 *  @since 2.7
 * 
 *  @note Please be sure not to use this method to pass any private or confidential information
 *  about the user.
 *
 *  @param userID The app id for a user.
 */
+ (void)setUserID:(NSString *)userID;

/*!
 *  @brief Set your user's age in years.
 *  @since 2.7
 * 
 *  Use this method to capture the age of your user. Only use this method if you collect this
 *  information explictly from your user (i.e. - there is no need to set a default value).
 *
 *  @note The age is aggregated across all users of your app and not available on a per user
 *  basis.
 *
 *  @param age Reported age of user.
 *
 */
+ (void)setAge:(int)age;

/*!
 *  @brief Set your user's gender.
 *  @since 2.7
 * 
 *  Use this method to capture the gender of your user. Only use this method if you collect this
 *  information explictly from your user (i.e. - there is no need to set a default value). Allowable
 *  values are @c @"m" or @c @"f"
 *
 *  @note The gender is aggregated across all users of your app and not available on a per user
 *  basis.
 *
 *  @param gender Reported gender of user.
 *
 */
+ (void)setGender:(NSString *)gender;	// user's gender m or f

//@}

/** @name Location Reporting
 *  Methods for setting location information. 
 */
//@{
/*!
 *  @brief Set the location of the session.
 *  @since 2.7
 * 
 *  Use information from the CLLocationManager to specify the location of the session. Flurry does not
 *  automatically track this information or include the CLLocation framework.
 *
 *  @note Only the last location entered is captured per session. \n
 *  Regardless of accuracy specified, the Flurry SDK will only report location at city level or higher. \n
 *  Location is aggregated across all users of your app and not available on a per user basis. \n
 *  This information should only be captured if it is germaine to the use of your app.
 *
 *  @code
    CLLocationManager *locationManager = [[CLLocationManager alloc] init];
    [locationManager startUpdatingLocation];
 *  @endcode
 *
 *  After starting the location manager, you can set the location with Flurry. You can implement
 *  CLLocationManagerDelegate to be aware of when the location is updated. Below is an example 
 *  of how to use this method, after you have recieved a location update from the locationManager.
 *
 *  @code
    CLLocation *location = locationManager.location;
        [Flurry  setLatitude:location.coordinate.latitude
                   longitude:location.coordinate.longitude
          horizontalAccuracy:location.horizontalAccuracy
            verticalAccuracy:location.verticalAccuracy];
 *  @endcode
 *  @param latitude The latitude.
 *  @param longitude The longitude.
 *  @param horizontalAccuracy The radius of uncertainty for the location in meters.
 *  @param verticalAccuracy The accuracy of the altitude value in meters.
 *
 */
+ (void)setLatitude:(double)latitude
             longitude:(double)longitude
    horizontalAccuracy:(float)horizontalAccuracy
      verticalAccuracy:(float)verticalAccuracy __attribute__((deprecated));
;

//@}

/** @name Location Reporting
 *  Opt-out Methods for setting location information.
 */
//@{
/*!
 *  @brief Turn on/off location information of the session (default is on).
 *  @since 8.4.0
 *
 *  Use CLLocationManager to start the location tracking of the session. Flurry does not
 *  prompt users for location permission, we fetch the available location in device daemon.
 *
 *  @note Only the last location in cache is captured per session. \n
 *  Regardless of accuracy specified, the Flurry SDK will only report location at city level or
 higher. \n
 *  Location is aggregated across all users of your app and not available on a per user basis. \n
 *  This information should only be captured if it is germaine to the use of your app.
 *
 *  @code
 CLLocationManager *locationManager = [[CLLocationManager alloc] init];
 [locationManager startUpdatingLocation];
 *  @endcode
 *
 *  It is on by default. After starting the location manager, you can turn off
 *  opt-out location tracking by calling this method.
 *
 *  @code
 [Flurry  trackPreciseLocation:NO];
 *  @endcode
 *  @param state The boolean to switch on/off for location tracking
 *  @return a boolean, if the state is YES, it tests device/app permission. If permission
 *   is granted, it returns NO. If permission is valid, it returns YES. If the
 *   state is NO, it always returns NO.
 */
+ (BOOL)trackPreciseLocation:(BOOL)state;

//@}

/** @name Session Reporting Calls
 *  Optional methods that can be called at any point to control session reporting. 
 */
//@{

/*!
 *  @brief Set session to report when app closes.
 *  @since 2.7
 * 
 *  Use this method report session data when the app is closed. The default value is @c YES.
 *
 *  @note This method is rarely invoked in iOS >= 3.2 due to the updated iOS lifecycle.
 *
 *  @see #setSessionReportsOnPauseEnabled:
 *
 *  @param sendSessionReportsOnClose YES to send on close, NO to omit reporting on close.
 *
 */
+ (void)setSessionReportsOnCloseEnabled:(BOOL)sendSessionReportsOnClose;

/*!
 *  @brief Set session to report when app is sent to the background.
 *  @since 2.7
 * 
 *  Use this method report session data when the app is paused. The default value is @c YES.
 *
 *  @param setSessionReportsOnPauseEnabled YES to send on pause, NO to omit reporting on pause.
 *
 */
+ (void)setSessionReportsOnPauseEnabled:(BOOL)setSessionReportsOnPauseEnabled;

/*!
 *  @brief Set session to support background execution.
 *  @since 4.2.2
 *
 *  Use this method to enable reporting of errors and events when application is 
 *  running in backgorund (such applications have  UIBackgroundModes in Info.plist).
 *  You should call #pauseBackgroundSession when appropriate in background mode to 
 *  pause the session (for example when played song completed in background)
 *
 *  Default value is @c NO
 *
 *  @see #pauseBackgroundSession for details
 *
 *  @param setBackgroundSessionEnabled YES to enbale background support and 
 *  continue log events and errors for running session.
 */
+ (void)setBackgroundSessionEnabled:(BOOL)setBackgroundSessionEnabled;

/*!
 *  @brief Enable custom event logging.
 *  @since 2.7
 *
 *  @deprecated since 7.9.0.
 *  This method will be removed in a future version of the SDK.
 *
 *  Use this method to allow the capture of custom events. The default value is @c YES.
 *
 *  @param value YES to enable event logging, NO to stop custom logging.
 *
 */
+ (void)setEventLoggingEnabled:(BOOL)value __attribute__((deprecated));

#if !TARGET_OS_TV
/*!
 *  @brief Enables Flurry Pulse
 *  @since 6.3.0
 *
 *  @note: Please see https://developer.yahoo.com/flurry-pulse/ for more details
 *
 *  @param value YES to enable event logging, NO to stop custom logging.
 *
 */
+ (void)setPulseEnabled:(BOOL)value;
#endif


/*!
 *  @brief Records a syndicated event specified by @c syndicationEvent.
 *  @since 6.7.0
 *
 *  This method is excusively for use by the Tumblr App, calls from others app will be ignored.
 *
 *  @code
    - (void) reblogButtonHandler
    {
        [Flurry logEvent:Reblog syndicationID:@"123", parameters:nil];
        // Perform
    }
 *  @endcode
 *
 *  @param syndicationEvent syndication event.
 *  @param syndicationID syndication ID that is associated with the event
 *  @param parameters use this to pass in syndication parameters such as
 *         kSyndicationiOSDeepLink, kSyndicationAndroidDeepLink, kSyndicationWebLinkDeepLink
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus) logEvent:(FlurrySyndicationEvent) syndicationEvent syndicationID:(NSString*) syndicationID parameters:(NSDictionary*) parameters;

#if !TARGET_OS_WATCH
/*!
 *  @brief Records an Apple Store transaction.
 *  @since 7.8.0
 *
 *  This method needs to be called before a transaction is finished and finalized.
 *  @note: Needs a 'required' dependency on StoreKit for this API to function correctly.
 *
 *  @param transaction an SKPaymentTransaction.
 *  @param statusCallback a callback gettign called when the status of  ID that is associated with
 * the event
 *
 */
+ (void) logPaymentTransaction:(SKPaymentTransaction*)transaction statusCallback:(void(^)(FlurryTransactionRecordStatus))statusCallback;
#endif

#if !TARGET_OS_WATCH
/*!
 *  @brief Enables implicit recording of Apple Store transactions.
 *  @since 7.9.0
 *
 *  This method needs to be called before any transaction is finialized.
 *  @note: Needs a 'required' dependency on StoreKit for this API to function correctly.
 *
 *  @param value YES to enable transaction logging, NO to stop transaction logging.
 *
 */
+ (void)setIAPReportingEnabled:(BOOL)value;
#endif

#if TARGET_OS_TV
/*!
 *  @brief Registers the TVML's JSContext with the Flurry SDK.
 *  @since 1.0.0
 *  
 *
 *  @param appController The TVApplicationController object
 *  @param jsContext The JavaScript context object passed in
 *
 *  This method is exclusively for use by the Client-Server TV apps. This method will internally register
 *  JavaScript APIs exposed on the TVJS domain with the Flurry SDK. The JavaScript methods available are:
 *  flurryLogEvent({String} eventName)
 *  flurryLogEvent({String} eventName, {object} params)
 *  flurryLogTimedEvent({String} eventName)
 *  flurryLogTimedEvent({String} eventName, {object} params)
 *  flurryEndTimedEvent({String} eventName, {object} params)
 *  flurryLogError({String} eventName, {String} message, {object} error)
 *  -> error : {
 *              errorDomain: {String},
                errorID: {Number},
                userInfo: {object}
 *              }
 *  flurrySetUserID({String} userID)
 *  flurrySetGender({String} gender)
 *  flurrySetAge({Number} age)
 *  flurrySetLocation({Number} latitude, {Number} longitude, {Number} horizontalAccuracy, {Number} verticalAccuracy)
 *
 *  @code
 - (void)appController:(TVApplicationController *)appController evaluateAppJavaScriptInContext:(JSContext *)jsContext {
    [Flurry registerJSContextWithContext:jsContext];
 }
 *  @endcode
 * 
 *  @param  jscontext JavaScript context passed in by the -appController:evaluateAppJavaScriptInContext method
 */
 
+ (void)registerJSContextWithContext:(JSContext*)jscontext;
#endif

@end
