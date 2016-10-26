/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *
 * Copyright (c) 2013-2014 HockeyApp, Bit Stadium GmbH.
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


#import <UIKit/UIKit.h>
#import "BITHockeyBaseManager.h"


/**
 *  Defines the update check intervals
 */
typedef NS_ENUM(NSInteger, BITStoreUpdateSetting) {
  /**
   *  Check every day
   */
  BITStoreUpdateCheckDaily = 0,
  /**
   *  Check every week
   */
  BITStoreUpdateCheckWeekly = 1,
  /**
   *  Check manually
   */
  BITStoreUpdateCheckManually = 2
};

@protocol BITStoreUpdateManagerDelegate;

/**
 The store update manager module.

 This is the HockeySDK module for handling app updates when having your app released in the App Store.
 By default the module uses the current users locale to define the app store to check for updates. You
 can modify this using the `countryCode` property. See the property documentation for details on its usage.

 When an update is detected, this module will show an alert asking the user if he/she wants to update or
 ignore this version. If update was chosen, it will open the apps page in the app store app.

 You need to enable this module using `[BITHockeyManager enableStoreUpdateManager]` if you want to use this
 feature. By default this module is disabled!

 When this module is enabled and **NOT** running in an App Store build/environment, it won't do any checks!

 The `BITStoreUpdateManagerDelegate` protocol informs the app about new detected app versions.

 @warning This module can **NOT** check if the current device and OS version match the minimum requirements of
 the new app version!

 */

@interface BITStoreUpdateManager : BITHockeyBaseManager

///-----------------------------------------------------------------------------
/// @name Update Checking
///-----------------------------------------------------------------------------

/**
 When to check for new updates.

 Defines when a the SDK should check if there is a new update available on the
 server. This must be assigned one of the following, see `BITStoreUpdateSetting`:

 - `BITStoreUpdateCheckDaily`: Once a day
 - `BITStoreUpdateCheckWeekly`: Once a week
 - `BITStoreUpdateCheckManually`: Manually

 **Default**: BITStoreUpdateCheckWeekly

 @warning When setting this to `BITStoreUpdateCheckManually` you need to either
 invoke the update checking process yourself with `checkForUpdate` somehow, e.g. by
 proving an update check button for the user or integrating the Update View into your
 user interface.
 @see BITStoreUpdateSetting
 @see countryCode
 @see checkForUpdateOnLaunch
 @see checkForUpdate
 */
@property (nonatomic, assign) BITStoreUpdateSetting updateSetting;


/**
 Defines the store country the app is always available in, otherwise uses the users locale

 If this value is not defined, then it uses the device country if the current locale.

 If you are pre-defining a country and are releasing a new version on a specific date,
 it can happen that users get an alert but the update is not yet available in their country!

 But if a user downloaded the app from another appstore than the locale is set and the app is not
 available in the locales app store, then the user will never receive an update notification!

 More information about possible country codes is available here: http://en.wikipedia.org/wiki/ISO_3166-1_alpha-2

 @see updateSetting
 @see checkForUpdateOnLaunch
 @see checkForUpdate
 */
@property (nonatomic, strong) NSString *countryCode;


/**
 Flag that determines whether the automatic update checks should be done.

 If this is enabled the update checks will be performed automatically depending on the
 `updateSetting` property. If this is disabled the `updateSetting` property will have
 no effect, and checking for updates is totally up to be done by yourself.

 *Default*: _YES_

 @warning When setting this to `NO` you need to invoke update checks yourself!
 @see updateSetting
 @see countryCode
 @see checkForUpdate
 */
@property (nonatomic, assign, getter=isCheckingForUpdateOnLaunch) BOOL checkForUpdateOnLaunch;


///-----------------------------------------------------------------------------
/// @name User Interface
///-----------------------------------------------------------------------------


/**
 Flag that determines if the integrated update alert should be used

 If enabled, the integrated UIAlert based update notification will be used to inform
 the user about a new update being available in the App Store.

 If disabled, you need to implement the `BITStoreUpdateManagerDelegate` protocol with
 the method `[BITStoreUpdateManagerDelegate detectedUpdateFromStoreUpdateManager:newVersion:storeURL:]`
 to be notified about new version and proceed yourself.
 The manager will consider this identical to an `Ignore` user action using the alert
 and not inform about this particular version any more, unless the app is updated
 and this very same version shows up at a later time again as a new version.

 *Default*: _YES_

 @warning If the HockeySDKResources bundle is missing in the application package, then the internal
 update alert is also disabled and be treated identical to manually disabling this
 property.
 @see updateSetting
 */
@property (nonatomic, assign, getter=isUpdateUIEnabled) BOOL updateUIEnabled;

///-----------------------------------------------------------------------------
/// @name Manual update checking
///-----------------------------------------------------------------------------

/**
 Check for an update

 Call this to trigger a check if there is a new update available on the HockeyApp servers.

 @see updateSetting
 @see countryCode
 @see checkForUpdateOnLaunch
 */
- (void)checkForUpdate;


@end
