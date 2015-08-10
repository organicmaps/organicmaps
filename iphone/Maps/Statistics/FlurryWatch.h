//
//  FlurryWatch.h
//  Flurry iOS Analytics Agent
//
//  Copyright 2009-2015 Flurry, Inc. All rights reserved.
//
//	Methods in this header file are for use with Flurry Analytics

#import "Flurry.h"

@interface FlurryWatch : NSObject

/*!
 *  @brief Records a custom event specified by @c eventName.
 *  @since 6.4.0
 *
 *  This method allows you to specify custom watch events within your watch extension.
 *  As a general rule you should capture events related to user navigation within your
 *  app, any actionaround monetization, and other events as they are applicable to
 *  tracking progress towards your business goals.
 *
 *  @note You should not pass private or confidential information about your users in a
 *  custom event. \n
 *  This method is only supported within a watch extension.\n
 *
 *  @see #logWatchEvent:withParameters: for details on storing events with parameters. \n
 *
 *  @code
 *  - (void)interestingAppAction
 {
 [FlurryWatch logWatchEvent:@"Interesting_Action"];
 // Perform interesting action
 }
 *  @endcode
 *
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logWatchEvent:(NSString *)eventName;

/*!
 *  @brief Records a custom parameterized event specified by @c eventName with @c parameters.
 *  @since 6.4.0
 *
 *  This method overloads #logWatchEvent to allow you to associate parameters with an event. Parameters
 *  are extremely valuable as they allow you to store characteristics of an action. For example,
 *  if a user clicked a confirmation button, it may be useful to know the reservation details.
 *  By setting this parameter you will be able to view a distribution of reservations
 *  on the <a href="http://dev.flurry.com">Flurrly Dev Portal</a>.
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
 *  This method is only supported within a watch extension.\n
 *
 *  @code
 *  - (void)userConfirmedTheReservation
 {
 NSDictionary *params =
 [NSDictionary dictionaryWithObjectsAndKeys:@"Great Restaurant", // Parameter Value
 @"Destination", // Parameter Name
 nil];
 [FlurryWatch logWatchEvent:@"Reservation Confirmed" withParameters:params];
 // Confirm the reservation
 }
 *  @endcode
 *
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters An immutable copy of map containing Name-Value pairs of parameters.
 *
 *  @return enum FlurryEventRecordStatus for the recording status of the logged event.
 */
+ (FlurryEventRecordStatus)logWatchEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;

/*!
 *  @brief Records a watch exception. Commonly used to catch unhandled exceptions.
 *  @since 6.4.0
 *
 *  This method captures an exception for reporting to Flurry. We recommend adding an uncaught
 *  exception listener to capture any exceptions that occur during usage that is not
 *  anticipated by your app.
 *
 *  @note This method is only supported within a watch extension.\n
 *
 *  @see #logWatchError:message:error: for details on capturing errors.
 *
 *  @code
 *  - (void) uncaughtExceptionHandler(NSException *exception)
 {
 [FlurryWatch logWatchError:@"Uncaught" message:@"Crash!" exception:exception];
 }
 *  @endcode
 *
 *  @param errorID Name of the error.
 *  @param message The message to associate with the error.
 *  @param exception The exception object to report.
 */
+ (void)logWatchError:(NSString *)errorID message:(NSString *)message exception:(NSException *)exception;

/*!
 *  @brief Records a watch error.
 *  @since 6.4.0
 *
 *  This method captures an error for reporting to Flurry.
 *
 *  @note This method is only supported within a watch extension.\n
 *
 *  @see #logWatchError:message:exception: for details on capturing exceptions.
 *
 *  @code
 *  - (void) webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
 {
 [FlurryWatch logWatchError:@"Interface failed to load" message:[error localizedDescription] error:error];
 }
 *  @endcode
 *
 *  @param errorID Name of the error.
 *  @param message The message to associate with the error.
 *  @param error The error object to report.
 */
+ (void)logWatchError:(NSString *)errorID message:(NSString *)message error:(NSError *)error;

@end
