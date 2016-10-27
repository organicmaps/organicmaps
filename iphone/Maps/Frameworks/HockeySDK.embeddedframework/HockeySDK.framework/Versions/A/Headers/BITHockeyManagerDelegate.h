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
#import "HockeySDKFeatureConfig.h"

#if HOCKEYSDK_FEATURE_CRASH_REPORTER
#import "BITCrashManagerDelegate.h"
#endif

#if HOCKEYSDK_FEATURE_UPDATES
#import "BITUpdateManagerDelegate.h"
#endif

#if HOCKEYSDK_FEATURE_FEEDBACK
#import "BITFeedbackManagerDelegate.h"
#endif

#if HOCKEYSDK_FEATURE_AUTHENTICATOR
#import "BITAuthenticator.h"
#endif

@class BITHockeyManager;
@class BITHockeyBaseManager;

/**
 The `BITHockeyManagerDelegate` formal protocol defines methods further configuring
  the behaviour of `BITHockeyManager`, as well as the delegate of the modules it manages.
 */

@protocol BITHockeyManagerDelegate <NSObject
#if HOCKEYSDK_FEATURE_CRASH_REPORTER
  , BITCrashManagerDelegate
#endif
#if HOCKEYSDK_FEATURE_UPDATES
  , BITUpdateManagerDelegate
#endif
#if HOCKEYSDK_FEATURE_FEEDBACK
  , BITFeedbackManagerDelegate
#endif
#if HOCKEYSDK_FEATURE_AUTHENTICATOR
  , BITAuthenticatorDelegate
#endif
  >

@optional


///-----------------------------------------------------------------------------
/// @name App Identifier usage
///-----------------------------------------------------------------------------

/**
 Implement to force the usage of the live identifier

 This is useful if you are e.g. distributing an enterprise app inside your company
 and want to use the `liveIdentifier` for that even though it is not running from
 the App Store.

 Example:

     - (BOOL)shouldUseLiveIdentifierForHockeyManager:(BITHockeyManager *)hockeyManager {
     #ifdef (CONFIGURATION_AppStore)
       return YES;
     #endif
       return NO;
     }

 @param hockeyManager BITHockeyManager instance
 */
- (BOOL)shouldUseLiveIdentifierForHockeyManager:(BITHockeyManager *)hockeyManager;


///-----------------------------------------------------------------------------
/// @name UI presentation
///-----------------------------------------------------------------------------


// optional parent view controller for the feedback screen when invoked via the alert view, default is the root UIWindow instance
/**
 Return a custom parent view controller for presenting modal sheets

 By default the SDK is using the root UIWindow instance to present any required
 view controllers. Overwrite this if this doesn't result in a satisfying
 behavior or if you want to define any other parent view controller.

 @param hockeyManager The `BITHockeyManager` HockeyManager instance invoking this delegate
 @param componentManager The `BITHockeyBaseManager` component instance invoking this delegate, can be `BITCrashManager` or `BITFeedbackManager`
 */
- (UIViewController *)viewControllerForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager;


///-----------------------------------------------------------------------------
/// @name Additional meta data
///-----------------------------------------------------------------------------


/** Return the userid that should used in the SDK components

 Right now this is used by the `BITCrashManager` to attach to a crash report.
 `BITFeedbackManager` uses it too for assigning the user to a discussion thread.

 In addition, if this returns not nil for `BITFeedbackManager` the user will
 not be asked for any user details by the component, including userName or userEmail.

 You can find out the component requesting the userID like this:

    - (NSString *)userIDForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager {
      if (componentManager == hockeyManager.feedbackManager) {
        return UserIDForFeedback;
      } else if (componentManager == hockeyManager.crashManager) {
        return UserIDForCrashReports;
      } else {
        return nil;
      }
    }

 For crash reports, this delegate is invoked on the startup after the crash!

 Alternatively you can also use `[BITHockeyManager userID]` which will cache the value in the keychain.

 @warning When returning a non nil value for the `BITCrashManager` component, crash reports
 are not anonymous any more and the crash alerts will not show the word "anonymous"!

 @param hockeyManager The `BITHockeyManager` HockeyManager instance invoking this delegate
 @param componentManager The `BITHockeyBaseManager` component instance invoking this delegate, can be `BITCrashManager` or `BITFeedbackManager`
 @see userNameForHockeyManager:componentManager:
 @see userEmailForHockeyManager:componentManager:
 @see [BITHockeyManager userID]
 */
- (NSString *)userIDForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager;


/** Return the user name that should used in the SDK components

 Right now this is used by the `BITCrashManager` to attach to a crash report.
 `BITFeedbackManager` uses it too for assigning the user to a discussion thread.

 In addition, if this returns not nil for `BITFeedbackManager` the user will
 not be asked for any user details by the component, including userName or userEmail.

 You can find out the component requesting the user name like this:

    - (NSString *)userNameForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager {
      if (componentManager == hockeyManager.feedbackManager) {
        return UserNameForFeedback;
      } else if (componentManager == hockeyManager.crashManager) {
        return UserNameForCrashReports;
      } else {
        return nil;
      }
 }

 For crash reports, this delegate is invoked on the startup after the crash!

 Alternatively you can also use `[BITHockeyManager userName]` which will cache the value in the keychain.

 @warning When returning a non nil value for the `BITCrashManager` component, crash reports
 are not anonymous any more and the crash alerts will not show the word "anonymous"!

 @param hockeyManager The `BITHockeyManager` HockeyManager instance invoking this delegate
 @param componentManager The `BITHockeyBaseManager` component instance invoking this delegate, can be `BITCrashManager` or `BITFeedbackManager`
 @see userIDForHockeyManager:componentManager:
 @see userEmailForHockeyManager:componentManager:
 @see [BITHockeyManager userName]
 */
- (NSString *)userNameForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager;


/** Return the users email address that should used in the SDK components

 Right now this is used by the `BITCrashManager` to attach to a crash report.
 `BITFeedbackManager` uses it too for assigning the user to a discussion thread.

 In addition, if this returns not nil for `BITFeedbackManager` the user will
 not be asked for any user details by the component, including userName or userEmail.

 You can find out the component requesting the user email like this:

    - (NSString *)userEmailForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager {
      if (componentManager == hockeyManager.feedbackManager) {
        return UserEmailForFeedback;
      } else if (componentManager == hockeyManager.crashManager) {
        return UserEmailForCrashReports;
       } else {
        return nil;
       }
    }

 For crash reports, this delegate is invoked on the startup after the crash!

 Alternatively you can also use `[BITHockeyManager userEmail]` which will cache the value in the keychain.

 @warning When returning a non nil value for the `BITCrashManager` component, crash reports
 are not anonymous any more and the crash alerts will not show the word "anonymous"!

 @param hockeyManager The `BITHockeyManager` HockeyManager instance invoking this delegate
 @param componentManager The `BITHockeyBaseManager` component instance invoking this delegate, can be `BITCrashManager` or `BITFeedbackManager`
 @see userIDForHockeyManager:componentManager:
 @see userNameForHockeyManager:componentManager:
 @see [BITHockeyManager userEmail]
 */
- (NSString *)userEmailForHockeyManager:(BITHockeyManager *)hockeyManager componentManager:(BITHockeyBaseManager *)componentManager;

@end
