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

@class BITUpdateManager;

/**
 The `BITUpdateManagerDelegate` formal protocol defines methods further configuring
 the behaviour of `BITUpdateManager`.
 */

@protocol BITUpdateManagerDelegate <NSObject>

@optional


///-----------------------------------------------------------------------------
/// @name Expiry
///-----------------------------------------------------------------------------

/**
 Return if expiry alert should be shown if date is reached

 If you want to display your own user interface when the expiry date is reached,
 implement this method, present your user interface and return _NO_. In this case
 it is your responsibility to make the app unusable!

 Note: This delegate will be invoked on startup and every time the app becomes
 active again!

 When returning _YES_ the default blocking UI will be shown.

 When running the app from the App Store, this delegate is ignored.

 @param updateManager The `BITUpdateManager` instance invoking this delegate
 @see [BITUpdateManager expiryDate]
 @see [BITUpdateManagerDelegate didDisplayExpiryAlertForUpdateManager:]
 */
- (BOOL)shouldDisplayExpiryAlertForUpdateManager:(BITUpdateManager *)updateManager;


/**
 Invoked once a default expiry alert is shown

 Once expiry date is reached and the default blocking UI is shown,
 this delegate method is invoked to provide you the possibility to do any
 desired additional processing.

 @param updateManager The `BITUpdateManager` instance invoking this delegate
 @see [BITUpdateManager expiryDate]
 @see [BITUpdateManagerDelegate shouldDisplayExpiryAlertForUpdateManager:]
 */
- (void)didDisplayExpiryAlertForUpdateManager:(BITUpdateManager *)updateManager;


///-----------------------------------------------------------------------------
/// @name Privacy
///-----------------------------------------------------------------------------

/** Return NO if usage data should not be send

 The update  module send usage data by default, if the application is _NOT_
 running in an App Store version. Implement this delegate and
 return NO if you want to disable this.

 If you intend to implement a user setting to let them enable or disable
 sending usage data, this delegate should be used to return that value.

 Usage data contains the following information:
 - App Version
 - iOS Version
 - Device type
 - Language
 - Installation timestamp
 - Usage time

 @param updateManager The `BITUpdateManager` instance invoking this delegate
 @warning When setting this to `NO`, you will _NOT_ know if this user is actually testing!
 */
- (BOOL)updateManagerShouldSendUsageData:(BITUpdateManager *)updateManager;


///-----------------------------------------------------------------------------
/// @name Privacy
///-----------------------------------------------------------------------------

/** Implement this method to be notified before an update starts.

 The update manager will send this delegate message _just_ before the system
 call to update the application is placed, but after the user has already chosen
 to install the update.

 There is no guarantee that the update will actually start after this delegate
 message is sent.

 @param updateManager The `BITUpdateManager` instance invoking this delegate
 */
- (BOOL)willStartDownloadAndUpdate:(BITUpdateManager *)updateManager;

/**
 Invoked right before the app will exit to allow app update to start (>= iOS8 only)

 The iOS installation mechanism only starts if the app the should be updated is currently
 not running. On all iOS versions up to iOS 7, the system did automatically exit the app
 in these cases. Since iOS 8 this isn't done any longer.

 @param updateManager The `BITUpdateManager` instance invoking this delegate
 */
- (void)updateManagerWillExitApp:(BITUpdateManager *)updateManager;


#pragma mark - Deprecated

///-----------------------------------------------------------------------------
/// @name Update View Presentation Helper
///-----------------------------------------------------------------------------

/**
 Provide a parent view controller for the update user interface

 If you don't have a `rootViewController` set on your `UIWindow` and the SDK cannot
 automatically find the current top most `UIViewController`, you can provide the
 `UIViewController` that should be used to present the update user interface modal.

 @param updateManager The `BITUpdateManager` instance invoking this delegate

 @deprecated Please use `BITHockeyManagerDelegate viewControllerForHockeyManager:componentManager:` instead
 */
- (UIViewController *)viewControllerForUpdateManager:(BITUpdateManager *)updateManager DEPRECATED_ATTRIBUTE;

@end
