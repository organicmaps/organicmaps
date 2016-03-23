/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *
 * Copyright (c) 2012-2014 HockeyApp, Bit Stadium GmbH.
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

/**
 *  Provides details about the crash that occurred in the previous app session
 */
@interface BITCrashDetails : NSObject

/**
 *  UUID for the crash report
 */
@property (nonatomic, readonly, strong) NSString *incidentIdentifier;

/**
 *  UUID for the app installation on the device
 */
@property (nonatomic, readonly, strong) NSString *reporterKey;

/**
 *  Signal that caused the crash
 */
@property (nonatomic, readonly, strong) NSString *signal;

/**
 *  Exception name that triggered the crash, nil if the crash was not caused by an exception
 */
@property (nonatomic, readonly, strong) NSString *exceptionName;

/**
 *  Exception reason, nil if the crash was not caused by an exception
 */
@property (nonatomic, readonly, strong) NSString *exceptionReason;

/**
 *  Date and time the app started, nil if unknown
 */
@property (nonatomic, readonly, strong) NSDate *appStartTime;

/**
 *  Date and time the crash occurred, nil if unknown
 */
@property (nonatomic, readonly, strong) NSDate *crashTime;

/**
 *  Operation System version string the app was running on when it crashed.
 */
@property (nonatomic, readonly, strong) NSString *osVersion;

/**
 *  Operation System build string the app was running on when it crashed
 *
 *  This may be unavailable.
 */
@property (nonatomic, readonly, strong) NSString *osBuild;

/**
 *  CFBundleShortVersionString value of the app that crashed
 *
 *  Can be `nil` if the crash was captured with an older version of the SDK
 *  or if the app doesn't set the value.
 */
@property (nonatomic, readonly, strong) NSString *appVersion;

/**
 *  CFBundleVersion value of the app that crashed
 */
@property (nonatomic, readonly, strong) NSString *appBuild;

/**
 *  Identifier of the app process that crashed
 */
@property (nonatomic, readonly, assign) NSUInteger appProcessIdentifier;

/**
 Indicates if the app was killed while being in foreground from the iOS

 If `[BITCrashManager enableAppNotTerminatingCleanlyDetection]` is enabled, use this on startup
 to check if the app starts the first time after it was killed by iOS in the previous session.

 This can happen if it consumed too much memory or the watchdog killed the app because it
 took too long to startup or blocks the main thread for too long, or other reasons. See Apple
 documentation: https://developer.apple.com/library/ios/qa/qa1693/_index.html

 See `[BITCrashManager enableAppNotTerminatingCleanlyDetection]` for more details about which kind of kills can be detected.

 @warning This property only has a correct value, once `[BITHockeyManager startManager]` was
 invoked! In addition, it is automatically disabled while a debugger session is active!

 @see `[BITCrashManager enableAppNotTerminatingCleanlyDetection]`
 @see `[BITCrashManager didReceiveMemoryWarningInLastSession]`

 @return YES if the details represent an app kill instead of a crash
 */
- (BOOL)isAppKill;

@end
