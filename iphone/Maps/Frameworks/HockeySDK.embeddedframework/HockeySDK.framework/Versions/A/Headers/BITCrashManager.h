/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *         Kent Sutherland
 *
 * Copyright (c) 2012-2014 HockeyApp, Bit Stadium GmbH.
 * Copyright (c) 2011 Andreas Linde & Kent Sutherland.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#import <Foundation/Foundation.h>

#import "BITHockeyBaseManager.h"

@class BITCrashDetails;
@class BITCrashMetaData;


/**
 * Custom block that handles the alert that prompts the user whether he wants to send crash reports
 */
typedef void(^BITCustomAlertViewHandler)();


/**
 * Crash Manager status
 */
typedef NS_ENUM(NSUInteger, BITCrashManagerStatus) {
  /**
   *	Crash reporting is disabled
   */
  BITCrashManagerStatusDisabled = 0,
  /**
   *	User is asked each time before sending
   */
  BITCrashManagerStatusAlwaysAsk = 1,
  /**
   *	Each crash report is send automatically
   */
  BITCrashManagerStatusAutoSend = 2
};


/**
 * Prototype of a callback function used to execute additional user code. Called upon completion of crash
 * handling, after the crash report has been written to disk.
 *
 * @param context The API client's supplied context value.
 *
 * @see `BITCrashManagerCallbacks`
 * @see `[BITCrashManager setCrashCallbacks:]`
 */
typedef void (*BITCrashManagerPostCrashSignalCallback)(void *context);

/**
 * This structure contains callbacks supported by `BITCrashManager` to allow the host application to perform
 * additional tasks prior to program termination after a crash has occurred.
 *
 * @see `BITCrashManagerPostCrashSignalCallback`
 * @see `[BITCrashManager setCrashCallbacks:]`
 */
typedef struct BITCrashManagerCallbacks {
  /** An arbitrary user-supplied context value. This value may be NULL. */
  void *context;

  /**
   * The callback used to report caught signal information.
   */
  BITCrashManagerPostCrashSignalCallback handleSignal;
} BITCrashManagerCallbacks;


/**
 * Crash Manager alert user input
 */
typedef NS_ENUM(NSUInteger, BITCrashManagerUserInput) {
  /**
   *  User chose not to send the crash report
   */
  BITCrashManagerUserInputDontSend = 0,
  /**
   *  User wants the crash report to be sent
   */
  BITCrashManagerUserInputSend = 1,
  /**
   *  User chose to always send crash reports
   */
  BITCrashManagerUserInputAlwaysSend = 2

};


@protocol BITCrashManagerDelegate;

/**
 The crash reporting module.

 This is the HockeySDK module for handling crash reports, including when distributed via the App Store.
 As a foundation it is using the open source, reliable and async-safe crash reporting framework
 [PLCrashReporter](https://code.google.com/p/plcrashreporter/).

 This module works as a wrapper around the underlying crash reporting framework and provides functionality to
 detect new crashes, queues them if networking is not available, present a user interface to approve sending
 the reports to the HockeyApp servers and more.

 It also provides options to add additional meta information to each crash report, like `userName`, `userEmail`
 via `BITHockeyManagerDelegate` protocol, and additional textual log information via `BITCrashManagerDelegate`
 protocol and a way to detect startup crashes so you can adjust your startup process to get these crash reports
 too and delay your app initialization.

 Crashes are send the next time the app starts. If `crashManagerStatus` is set to `BITCrashManagerStatusAutoSend`,
 crashes will be send without any user interaction, otherwise an alert will appear allowing the users to decide
 whether they want to send the report or not. This module is not sending the reports right when the crash happens
 deliberately, because if is not safe to implement such a mechanism while being async-safe (any Objective-C code
 is _NOT_ async-safe!) and not causing more danger like a deadlock of the device, than helping. We found that users
 do start the app again because most don't know what happened, and you will get by far most of the reports.

 Sending the reports on startup is done asynchronously (non-blocking). This is the only safe way to ensure
 that the app won't be possibly killed by the iOS watchdog process, because startup could take too long
 and the app could not react to any user input when network conditions are bad or connectivity might be
 very slow.

 It is possible to check upon startup if the app crashed before using `didCrashInLastSession` and also how much
 time passed between the app launch and the crash using `timeIntervalCrashInLastSessionOccurred`. This allows you
 to add additional code to your app delaying the app start until the crash has been successfully send if the crash
 occurred within a critical startup timeframe, e.g. after 10 seconds. The `BITCrashManagerDelegate` protocol provides
 various delegates to inform the app about it's current status so you can continue the remaining app startup setup
 after sending has been completed. The documentation contains a guide
 [How to handle Crashes on startup](HowTo-Handle-Crashes-On-Startup) with an example on how to do that.

 More background information on this topic can be found in the following blog post by Landon Fuller, the
 developer of [PLCrashReporter](https://www.plcrashreporter.org), about writing reliable and
 safe crash reporting: [Reliable Crash Reporting](http://goo.gl/WvTBR)

 @warning If you start the app with the Xcode debugger attached, detecting crashes will _NOT_ be enabled!
 */

@interface BITCrashManager : BITHockeyBaseManager


///-----------------------------------------------------------------------------
/// @name Configuration
///-----------------------------------------------------------------------------

/** Set the default status of the Crash Manager

 Defines if the crash reporting feature should be disabled, ask the user before
 sending each crash report or send crash reports automatically without
 asking.

 The default value is `BITCrashManagerStatusAlwaysAsk`. The user can switch to
 `BITCrashManagerStatusAutoSend` by choosing "Always" in the dialog (since
 `showAlwaysButton` default is _YES_).

 The current value is always stored in User Defaults with the key
 `BITCrashManagerStatus`.

 If you intend to implement a user setting to let them enable or disable
 crash reporting, this delegate should be used to return that value. You also
 have to make sure the new value is stored in the UserDefaults with the key
 `BITCrashManagerStatus`.

 @see BITCrashManagerStatus
 @see showAlwaysButton
 */
@property (nonatomic, assign) BITCrashManagerStatus crashManagerStatus;


/**
 *  Trap fatal signals via a Mach exception server.
 *
 *  By default the SDK is using the safe and proven in-process BSD Signals for catching crashes.
 *  This option provides an option to enable catching fatal signals via a Mach exception server
 *  instead.
 *
 *  We strongly advice _NOT_ to enable Mach exception handler in release versions of your apps!
 *
 *  Default: _NO_
 *
 * @warning The Mach exception handler executes in-process, and will interfere with debuggers when
 *  they attempt to suspend all active threads (which will include the Mach exception handler).
 *  Mach-based handling should _NOT_ be used when a debugger is attached. The SDK will not
 *  enabled catching exceptions if the app is started with the debugger running. If you attach
 *  the debugger during runtime, this may cause issues the Mach exception handler is enabled!
 * @see isDebuggerAttached
 */
@property (nonatomic, assign, getter=isMachExceptionHandlerEnabled) BOOL enableMachExceptionHandler;


/**
 *  Enable on device symbolication for system symbols
 *
 *  By default, the SDK does not symbolicate on the device, since this can
 *  take a few seconds at each crash. Also note that symbolication on the
 *  device might not be able to retrieve all symbols.
 *
 *  Enable if you want to analyze crashes on unreleased OS versions.
 *
 *  Default: _NO_
 */
@property (nonatomic, assign, getter=isOnDeviceSymbolicationEnabled) BOOL enableOnDeviceSymbolication;


/**
 *  EXPERIMENTAL: Enable heuristics to detect the app not terminating cleanly
 *
 *  This allows it to get a crash report if the app got killed while being in the foreground
 *  because of one of the following reasons:
 *  - The main thread was blocked for too long
 *  - The app took too long to start up
 *  - The app tried to allocate too much memory. If iOS did send a memory warning before killing the app because of this reason, `didReceiveMemoryWarningInLastSession` returns `YES`.
 *  - Permitted background duration if main thread is running in an endless loop
 *  - App failed to resume in time if main thread is running in an endless loop
 *  - If `enableMachExceptionHandler` is not activated, crashed due to stack overflow will also be reported
 *
 *  The following kills can _NOT_ be detected:
 *  - Terminating the app takes too long
 *  - Permitted background duration too long for all other cases
 *  - App failed to resume in time for all other cases
 *  - possibly more cases
 *
 *  Crash reports triggered by this mechanisms do _NOT_ contain any stack traces since the time of the kill
 *  cannot be intercepted and hence no stack trace of the time of the kill event can't be gathered.
 *
 *  The heuristic is implemented as follows:
 *  If the app never gets a `UIApplicationDidEnterBackgroundNotification` or `UIApplicationWillTerminateNotification`
 *  notification, PLCrashReporter doesn't detect a crash itself, and the app starts up again, it is assumed that
 *  the app got either killed by iOS while being in foreground or a crash occurred that couldn't be detected.
 *
 *  Default: _NO_
 *
 * @warning This is a heuristic and it _MAY_ report false positives! It has been tested with iOS 6.1 and iOS 7.
 * Depending on Apple changing notification events, new iOS version may cause more false positives!
 *
 * @see lastSessionCrashDetails
 * @see didReceiveMemoryWarningInLastSession
 * @see `BITCrashManagerDelegate considerAppNotTerminatedCleanlyReportForCrashManager:`
 * @see [Apple Technical Note TN2151](https://developer.apple.com/library/ios/technotes/tn2151/_index.html)
 * @see [Apple Technical Q&A QA1693](https://developer.apple.com/library/ios/qa/qa1693/_index.html)
 */
@property (nonatomic, assign, getter = isAppNotTerminatingCleanlyDetectionEnabled) BOOL enableAppNotTerminatingCleanlyDetection;


/**
 * Set the callbacks that will be executed prior to program termination after a crash has occurred
 *
 * PLCrashReporter provides support for executing an application specified function in the context
 * of the crash reporter's signal handler, after the crash report has been written to disk.
 *
 * Writing code intended for execution inside of a signal handler is exceptionally difficult, and is _NOT_ recommended!
 *
 * _Program Flow and Signal Handlers_
 *
 * When the signal handler is called the normal flow of the program is interrupted, and your program is an unknown state. Locks may be held, the heap may be corrupt (or in the process of being updated), and your signal handler may invoke a function that was being executed at the time of the signal. This may result in deadlocks, data corruption, and program termination.
 *
 * _Async-Safe Functions_
 *
 * A subset of functions are defined to be async-safe by the OS, and are safely callable from within a signal handler. If you do implement a custom post-crash handler, it must be async-safe. A table of POSIX-defined async-safe functions and additional information is available from the [CERT programming guide - SIG30-C](https://www.securecoding.cert.org/confluence/display/seccode/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers).
 *
 * Most notably, the Objective-C runtime itself is not async-safe, and Objective-C may not be used within a signal handler.
 *
 * Documentation taken from PLCrashReporter: https://www.plcrashreporter.org/documentation/api/v1.2-rc2/async_safety.html
 *
 * @see BITCrashManagerPostCrashSignalCallback
 * @see BITCrashManagerCallbacks
 *
 * @param callbacks A pointer to an initialized PLCrashReporterCallback structure, see https://www.plcrashreporter.org/documentation/api/v1.2-rc2/struct_p_l_crash_reporter_callbacks.html
 */
- (void)setCrashCallbacks: (BITCrashManagerCallbacks *) callbacks;


/**
 Flag that determines if an "Always" option should be shown

 If enabled the crash reporting alert will also present an "Always" option, so
 the user doesn't have to approve every single crash over and over again.

 If If `crashManagerStatus` is set to `BITCrashManagerStatusAutoSend`, this property
 has no effect, since no alert will be presented.

 Default: _YES_

 @see crashManagerStatus
 */
@property (nonatomic, assign, getter=shouldShowAlwaysButton) BOOL showAlwaysButton;


///-----------------------------------------------------------------------------
/// @name Crash Meta Information
///-----------------------------------------------------------------------------

/**
 Indicates if the app crash in the previous session

 Use this on startup, to check if the app starts the first time after it crashed
 previously. You can use this also to disable specific events, like asking
 the user to rate your app.

 @warning This property only has a correct value, once `[BITHockeyManager startManager]` was
 invoked!

 @see lastSessionCrashDetails
 */
@property (nonatomic, readonly) BOOL didCrashInLastSession;

/**
 Provides an interface to pass user input from a custom alert to a crash report

 @param userInput Defines the users action wether to send, always send, or not to send the crash report.
 @param userProvidedMetaData The content of this optional BITCrashMetaData instance will be attached to the crash report and allows to ask the user for e.g. additional comments or info.

 @return Returns YES if the input is a valid option and successfully triggered further processing of the crash report

 @see BITCrashManagerUserInput
 @see BITCrashMetaData
 */
- (BOOL)handleUserInput:(BITCrashManagerUserInput)userInput withUserProvidedMetaData:(BITCrashMetaData *)userProvidedMetaData;

/**
 Lets you set a custom block which handles showing a custom UI and asking the user
 whether he wants to send the crash report.

 This replaces the default alert the SDK would show!

 You can use this to present any kind of user interface which asks the user for additional information,
 e.g. what they did in the app before the app crashed.

 In addition to this you should always ask your users if they agree to send crash reports, send them
 always or not and return the result when calling `handleUserInput:withUserProvidedCrashDescription`.

 @param alertViewHandler A block that is responsible for loading, presenting and and dismissing your custom user interface which prompts the user if he wants to send crash reports. The block is also responsible for triggering further processing of the crash reports.

 @warning This is not available when compiled for Watch OS!

 @warning Block needs to call the `[BITCrashManager handleUserInput:withUserProvidedMetaData:]` method!

 @warning This needs to be set before calling `[BITHockeyManager startManager]`!
 */
- (void)setAlertViewHandler:(BITCustomAlertViewHandler)alertViewHandler;

/**
 * Provides details about the crash that occurred in the last app session
 */
@property (nonatomic, readonly) BITCrashDetails *lastSessionCrashDetails;


/**
 Indicates if the app did receive a low memory warning in the last session

 It may happen that low memory warning where send but couldn't be logged, since iOS
 killed the app before updating the flag in the filesystem did complete.

 This property may be true in case of low memory kills, but it doesn't have to be! Apps
 can also be killed without the app ever receiving a low memory warning.

 Also the app could have received a low memory warning, but the reason for being killed was
 actually different.

 @warning This property only has a correct value, once `[BITHockeyManager startManager]` was
 invoked!

 @see enableAppNotTerminatingCleanlyDetection
 @see lastSessionCrashDetails
 */
@property (nonatomic, readonly) BOOL didReceiveMemoryWarningInLastSession;


/**
 Provides the time between startup and crash in seconds

 Use this in together with `didCrashInLastSession` to detect if the app crashed very
 early after startup. This can be used to delay app initialization until the crash
 report has been sent to the server or if you want to do any other actions like
 cleaning up some cache data etc.

 Note that sending a crash reports starts as early as 1.5 seconds after the application
 did finish launching!

 The `BITCrashManagerDelegate` protocol provides some delegates to inform if sending
 a crash report was finished successfully, ended in error or was cancelled by the user.

 *Default*: _-1_
 @see didCrashInLastSession
 @see BITCrashManagerDelegate
 */
@property (nonatomic, readonly) NSTimeInterval timeIntervalCrashInLastSessionOccurred;


///-----------------------------------------------------------------------------
/// @name Helper
///-----------------------------------------------------------------------------

/**
 *  Detect if a debugger is attached to the app process
 *
 *  This is only invoked once on app startup and can not detect if the debugger is being
 *  attached during runtime!
 *
 *  @return BOOL if the debugger is attached on app startup
 */
- (BOOL)isDebuggerAttached;


/**
 * Lets the app crash for easy testing of the SDK
 *
 * The best way to use this is to trigger the crash with a button action.
 *
 * Make sure not to let the app crash in `applicationDidFinishLaunching` or any other
 * startup method! Since otherwise the app would crash before the SDK could process it.
 *
 * Note that our SDK provides support for handling crashes that happen early on startup.
 * Check the documentation for more information on how to use this.
 *
 * If the SDK detects an App Store environment, it will _NOT_ cause the app to crash!
 */
- (void)generateTestCrash;

///-----------------------------------------------------------------------------
/// @name Deprecated
///-----------------------------------------------------------------------------

@property (nonatomic, readonly) NSTimeInterval timeintervalCrashInLastSessionOccured DEPRECATED_MSG_ATTRIBUTE("Use the properly spelled property `timeIntervalCrashInLastSessionOccurred` instead.");

@end
