/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *
 * Copyright (c) 2012-2014 HockeyApp, Bit Stadium GmbH.
 * Copyright (c) 2011 Andreas Linde.
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


#if !defined (TARGET_OS_IOS) // Defined starting in iOS 9
#define TARGET_OS_IOS 1
#endif


#import "HockeySDKFeatureConfig.h"
#import "HockeySDKEnums.h"
#import "HockeySDKNullability.h"

#import "BITHockeyManager.h"
#import "BITHockeyManagerDelegate.h"

#if HOCKEYSDK_FEATURE_CRASH_REPORTER || HOCKEYSDK_FEATURE_FEEDBACK
#import "BITHockeyAttachment.h"
#endif

#if HOCKEYSDK_FEATURE_CRASH_REPORTER
#import "BITCrashManager.h"
#import "BITCrashAttachment.h"
#import "BITCrashManagerDelegate.h"
#import "BITCrashDetails.h"
#import "BITCrashMetaData.h"
#endif /* HOCKEYSDK_FEATURE_CRASH_REPORTER */

#if HOCKEYSDK_FEATURE_UPDATES
#import "BITUpdateManager.h"
#import "BITUpdateManagerDelegate.h"
#import "BITUpdateViewController.h"
#endif /* HOCKEYSDK_FEATURE_UPDATES */

#if HOCKEYSDK_FEATURE_STORE_UPDATES
#import "BITStoreUpdateManager.h"
#import "BITStoreUpdateManagerDelegate.h"
#endif /* HOCKEYSDK_FEATURE_STORE_UPDATES */

#if HOCKEYSDK_FEATURE_FEEDBACK
#import "BITFeedbackManager.h"
#import "BITFeedbackManagerDelegate.h"
#import "BITFeedbackActivity.h"
#import "BITFeedbackComposeViewController.h"
#import "BITFeedbackComposeViewControllerDelegate.h"
#import "BITFeedbackListViewController.h"
#endif /* HOCKEYSDK_FEATURE_FEEDBACK */

#if HOCKEYSDK_FEATURE_AUTHENTICATOR
#import "BITAuthenticator.h"
#endif

// Notification message which HockeyManager is listening to, to retry requesting updated from the server.
// This can be used by app developers to trigger additional points where the HockeySDK can try sending
// pending crash reports or feedback messages.
// By default the SDK retries sending pending data only when the app becomes active.
#define BITHockeyNetworkDidBecomeReachableNotification @"BITHockeyNetworkDidBecomeReachable"

extern NSString *const __attribute__((unused)) kBITCrashErrorDomain;
extern NSString *const __attribute__((unused)) kBITUpdateErrorDomain;
extern NSString *const __attribute__((unused)) kBITFeedbackErrorDomain;
extern NSString *const __attribute__((unused)) kBITAuthenticatorErrorDomain;
extern NSString *const __attribute__((unused)) kBITHockeyErrorDomain;
