//
//  FlurrySessionBuilder.h
//  Flurry
//
//  Created by Akshay Bhandary on 7/14/16.
//  Copyright © 2016 Flurry Inc. All rights reserved.
//

/*!
 *  @brief Enum for setting up log output level.
 *  @since 4.2.0
 *
 */
typedef enum {
    FlurryLogLevelNone = 0,         //No output
    FlurryLogLevelCriticalOnly,     //Default, outputs only critical log events
    FlurryLogLevelDebug,            //Debug level, outputs critical and main log events
    FlurryLogLevelAll               //Highest level, outputs all log events
} FlurryLogLevel;


@interface FlurrySessionBuilder : NSObject

/*!
 *  @brief Explicitly specifies the App Version that Flurry will use to group Analytics data.
 *  @since 7.7.0
 *
 *  This is an optional method that overrides the App Version Flurry uses for reporting. Flurry will
 *  use the CFBundleVersion in your info.plist file when this method is not invoked.
 *
 *  @note There is a maximum of 605 versions allowed for a single app.
 *
 *  @param value The custom version name.
 */
- (FlurrySessionBuilder*) withAppVersion:(NSString *)value;

/*!
 *  @brief Set the timeout for expiring a Flurry session.
 *  @since 7.7.0
 *
 *  This is an optional method that sets the time the app may be in the background before
 *  starting a new session upon resume.  The default value for the session timeout is 10
 *  seconds in the background.
 *
 *  @param value The time in seconds to set the session timeout to.
 */
- (FlurrySessionBuilder*) withSessionContinueSeconds:(NSInteger)value;


/*!
 *  @brief Enable automatic collection of crash reports.
 *  @since 7.7.0
 *
 *  This is an optional method that collects crash reports when enabled. The
 *  default value is @c NO.
 *
 *  @param value @c YES to enable collection of crash reports.
 */
- (FlurrySessionBuilder*) withCrashReporting:(BOOL)value;

/*!
 *  @brief Generates debug logs to console.
 *  @since 7.7.0
 *
 *  This is an optional method that displays debug information related to the Flurry SDK.
 *  display information to the console. The default setting for this method is @c FlurryLogLevelCriticalOnly.
 *
 *  @note The log level can be changed at any point in the execution of your application using the setLogLevel API defined in
 *  Flurry.h, see #setLogLevel for more info.
 *
 *  @param value Log level
 *
 */
- (FlurrySessionBuilder*) withLogLevel:(FlurryLogLevel) value;



/*!
 *  @brief Displays an exception in the debug log if thrown during a Session.
 *  @since 7.7.0
 *
 *  This is an optional method that augments the debug logs with exceptions that occur during the session.
 *  You must both capture exceptions to Flurry and set the log level to Debug or All for this method to
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
- (FlurrySessionBuilder*) withShowErrorInLog:(BOOL) value;

#if !TARGET_OS_WATCH
/*!
 *  @brief Enables implicit recording of Apple Store transactions.
 *  @since 7.9.0
 *
 *  @note This method needs to be called before any transaction is finialized.
 *
 *  @param value @c YES to enable transaction logging with the default being @c NO.
 *
 */

- (FlurrySessionBuilder *)withIAPReportingEnabled:(BOOL)value;

/*!
 *  @brief Enables opting out of background sessions being counted towards total sessions.
 *  @since 8.1.0-rc.1
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param value @c NO to opt out of counting background sessions towards total sessions.
 *  The default value for the session is @c YES
 *
 */

- (FlurrySessionBuilder *)withIncludeBackgroundSessionsInMetrics:(BOOL)value;
#endif

#if TARGET_OS_TV
/*!
 *  @brief Sets the minimum duration (in minutes) before a partial session report is sent to Flurry.
 *  @since 7.7.0
 *
 *  This is an optional method that sets the minimum duration (in minutes) before a partial session report is sent to Flurry.
 *  The acceptable values are between 5 and 60 minutes.
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param duration The period after which a partial session report is sent to Flurry.
 */
- (FlurrySessionBuilder*) withTVSessionReportingInterval:(NSInteger) value;

/*!
 *  @brief Sets the minimum number of events before a partial session report is sent to Flurry.
 *  @since 7.7.0
 *
 *  This is an optional method that sets the minimum number of events before a partial session report is sent to Flurry.
 *  The acceptable values are between 5 and 50.
 *
 *  @note This method must be called prior to invoking #startSession:.
 *
 *  @param  count The number of events after which partial session report is sent to Flurry.
 */
- (FlurrySessionBuilder*) withTVEventCountThreshold:(NSInteger) value;
#endif

@end
