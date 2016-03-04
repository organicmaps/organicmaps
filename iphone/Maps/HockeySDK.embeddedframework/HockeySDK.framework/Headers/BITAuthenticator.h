/*
 * Author: Stephan Diederich, Andreas Linde
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

#import <Foundation/Foundation.h>

#import "BITHockeyBaseManager.h"

/**
 * Identification Types
 */
typedef NS_ENUM(NSUInteger, BITAuthenticatorIdentificationType) {
  /**
   * Assigns this app an anonymous user id.
   * <br/><br/>
   * The user will not be asked anything and an anonymous ID will be generated.
   * This helps identifying this installation being unique but HockeyApp won't be able
   * to identify who actually is running this installation and on which device
   * the app is installed.
   */
  BITAuthenticatorIdentificationTypeAnonymous,
  /**
   * Ask for the HockeyApp account email
   * <br/><br/>
   * This will present a user interface requesting the user to provide their
   * HockeyApp user email address.
   * <br/><br/>
   * The provided email address has to match an email address of a registered
   * HockeyApp user who is a member or tester of the app
   */
  BITAuthenticatorIdentificationTypeHockeyAppEmail,
  /**
   * Ask for the HockeyApp account by email and password
   * <br/><br/>
   * This will present a user interface requesting the user to provide their
   * HockeyApp user credentials.
   * <br/><br/>
   * The provided user account has to match a registered HockeyApp user who is
   * a member or tester of the app
   */
  BITAuthenticatorIdentificationTypeHockeyAppUser,
  /**
   * Identifies the current device
   * <br/><br/>
   * This will open the HockeyApp web page on the device in Safari and request the user
   * to submit the device's unique identifier to the app. If the web page session is not aware
   * of the current devices UDID, it will request the user to install the HockeyApp web clip
   * which will provide the UDID to users session in the browser.
   * <br/><br/>
   * This requires the app to register an URL scheme. See the linked property and methods
   * for further documentation on this.
   */
  BITAuthenticatorIdentificationTypeDevice,
  /**
   * Ask for the HockeyApp account email.
   * <br/><br/>
   * This will present a user interface requesting the user to start a Safari based
   * flow to login to HockeyApp (if not already logged in) and to share the hockeyapp
   * account's email.
   * <br/><br/>
   * If restrictApplicationUsage is enabled, the provided user account has to match a
   * registered HockeyApp user who is a member or tester of the app.
   * For identification purpose any HockeyApp user is allowed.
   */
  BITAuthenticatorIdentificationTypeWebAuth,
};

/**
 * Restriction enforcement styles
 *
 * Specifies how often the Authenticator checks if the user is allowed to use
 * this app.
 */
typedef NS_ENUM(NSUInteger, BITAuthenticatorAppRestrictionEnforcementFrequency) {
  /**
   * Checks if the user is allowed to use the app at the first time a version is started
   */
  BITAuthenticatorAppRestrictionEnforcementOnFirstLaunch,
  /**
   * Checks if the user is allowed to use the app every time the app becomes active
   */
  BITAuthenticatorAppRestrictionEnforcementOnAppActive,
};

@protocol BITAuthenticatorDelegate;


/**
 * Identify and authenticate users of Ad-Hoc or Enterprise builds
 *
 * `BITAuthenticator` serves 2 purposes:
 *
 * 1. Identifying who is running your Ad-Hoc or Enterprise builds
 *    `BITAuthenticator` provides an identifier for the rest of the HockeySDK
 *    to work with, e.g. in-app update checks and crash reports.
 *
 * 2. Optional regular checking if an identified user is still allowed
 *    to run this application. The `BITAuthenticator` can be used to make
 *    sure only users who are testers of your app are allowed to run it.
 *
 * This module automatically disables itself when running in an App Store build by default!
 *
 * @warning It is mandatory to call `authenticateInstallation` somewhen after calling
 * `[[BITHockeyManager sharedHockeyManager] startManager]` or fully customize the identification
 * and validation workflow yourself.
 * If your app shows a modal view on startup, make sure to call `authenticateInstallation`
 * either once your modal view is fully presented (e.g. its `viewDidLoad:` method is processed)
 * or once your modal view is dismissed.
 */
@interface BITAuthenticator : BITHockeyBaseManager

#pragma mark - Configuration


///-----------------------------------------------------------------------------
/// @name Configuration
///-----------------------------------------------------------------------------


/**
 * Defines the identification mechanism to be used
 *
 * _Default_: `BITAuthenticatorIdentificationTypeAnonymous`
 *
 * @see BITAuthenticatorIdentificationType
 */
@property (nonatomic, assign) BITAuthenticatorIdentificationType identificationType;


/**
 * Enables or disables checking if the user is allowed to run this app
 *
 * If disabled, the Authenticator never validates, besides initial identification,
 * if the user is allowed to run this application.
 *
 * If enabled, the Authenticator checks depending on `restrictionEnforcementFrequency`
 * if the user is allowed to use this application.
 *
 * Enabling this property and setting `identificationType` to `BITAuthenticatorIdentificationTypeHockeyAppEmail`,
 * `BITAuthenticatorIdentificationTypeHockeyAppUser` or `BITAuthenticatorIdentificationTypeWebAuth` also allows
 * to remove access for users by removing them from the app's users list on HockeyApp.
 *
 *  _Default_: `NO`
 *
 * @warning if `identificationType` is set to `BITAuthenticatorIdentificationTypeAnonymous`,
 *          this property has no effect.
 *
 * @see BITAuthenticatorIdentificationType
 * @see restrictionEnforcementFrequency
 */
@property (nonatomic, assign) BOOL restrictApplicationUsage;

/**
 * Defines how often the BITAuthenticator checks if the user is allowed
 * to run this application
 *
 * This requires `restrictApplicationUsage` to be enabled.
 *
 * _Default_: `BITAuthenticatorAppRestrictionEnforcementOnFirstLaunch`
 *
 * @see BITAuthenticatorAppRestrictionEnforcementFrequency
 * @see restrictApplicationUsage
 */
@property (nonatomic, assign) BITAuthenticatorAppRestrictionEnforcementFrequency restrictionEnforcementFrequency;

/**
 * The authentication secret from HockeyApp. To find the right secret,
 * click on your app on the HockeyApp dashboard, then on Show next to
 * "Secret:".
 *
 * This is only needed if `identificationType` is set to `BITAuthenticatorIdentificationTypeHockeyAppEmail`
 *
 * @see identificationType
 */
@property (nonatomic, copy) NSString *authenticationSecret;


#pragma mark - Device based identification

///-----------------------------------------------------------------------------
/// @name Device based identification
///-----------------------------------------------------------------------------


/**
 * The baseURL of the webpage the user is redirected to if `identificationType` is
 * set to `BITAuthenticatorIdentificationTypeDevice`; defaults to https://rink.hockeyapp.net.
 *
 * @see identificationType
 */
@property (nonatomic, strong) NSURL *webpageURL;

/**
 * URL to query the device's id via external webpage
 * Built with the baseURL set in `webpageURL`.
 */
- (NSURL*) deviceAuthenticationURL;

/**
 * The url-scheme used to identify via `BITAuthenticatorIdentificationTypeDevice`
 *
 * Please make sure that the URL scheme is unique and not shared with other apps.
 *
 * If set to nil, the default scheme is used which is `ha<APP_ID>`.
 *
 * @see identificationType
 * @see handleOpenURL:sourceApplication:annotation:
 */
@property (nonatomic, strong) NSString *urlScheme;

/**
 Should be used by the app-delegate to forward handle application:openURL:sourceApplication:annotation: calls.

 This is required if `identificationType` is set to `BITAuthenticatorIdentificationTypeDevice`.
 Your app needs to implement the default `ha<APP_ID>` URL scheme or register its own scheme
 via `urlScheme`.
 BITAuthenticator checks if the given URL is actually meant to be parsed by it and will
 return NO if it doesn't think so. It does this by checking the 'host'-part of the URL to be 'authorize', as well
 as checking the protocol part.
 Please make sure that if you're using a custom URL scheme, it does _not_ conflict with BITAuthenticator's.
 If BITAuthenticator thinks the URL was meant to be an authorization URL, but could not find a valid token, it will
 reset the stored identification token and state.

 Sample usage (in AppDelegate):

    - (BOOL)application:(UIApplication *)application
                openURL:(NSURL *)url
      sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
      if ([[BITHockeyManager sharedHockeyManager].authenticator handleOpenURL:url
                                                            sourceApplication:sourceApplication
                                                                   annotation:annotation]) {
        return YES;
      } else {
        //do your own URL handling, return appropriate value
      }
      return NO;
    }

 @param url Param `url` that was passed to the app
 @param sourceApplication Param `sourceApplication` that was passed to the app
 @param annotation Param `annotation` that was passed to the app

 @return YES if the URL request was handled, NO if the URL could not be handled/identified.

 @see identificationType
 @see urlScheme
 */
- (BOOL) handleOpenURL:(NSURL *) url
     sourceApplication:(NSString *) sourceApplication
            annotation:(id) annotation;

#pragma mark - Authentication

///-----------------------------------------------------------------------------
/// @name Authentication
///-----------------------------------------------------------------------------

/**
 * Invoked automatic identification and validation
 *
 * If the `BITAuthenticator` is in automatic mode this will initiate identifying
 * the current user according to the type specified in `identificationType` and
 * validate if the identified user is allowed to run this application.
 *
 * If the user is not yet identified it will present a modal view asking the user to
 * provide the required information.
 *
 * If your app provides it's own startup modal screen, e.g. a guide or a login, then
 * you might either call this method once that UI is fully presented or once
 * the user e.g. did actually login already.
 *
 * @warning You need to call this method in your code even if automatic mode is enabled!
 *
 * @see identificationType
 */
- (void) authenticateInstallation;

/**
 * Identifies the user according to the type specified in `identificationType`.
 *
 * If the `BITAuthenticator` is in manual mode, it's your responsibility to call
 * this method. Depending on the `identificationType`, this method
 * might present a viewController to let the user enter his/her credentials.
 *
 * If the Authenticator is in auto-mode, this is called by the authenticator itself
 * once needed.
 *
 * @see identificationType
 * @see authenticateInstallation
 * @see validateWithCompletion:
 *
 * @param completion Block being executed once identification completed. Be sure to properly dispatch code to the main queue if necessary.
 */
- (void) identifyWithCompletion:(void(^)(BOOL identified, NSError *error)) completion;

/**
 * Returns YES if this app is identified according to the setting in `identificationType`.
 *
 * Since the identification process is done asynchronously (contacting the server),
 * you need to observe the value change via KVO.
 *
 * @see identificationType
 */
@property (nonatomic, assign, readonly, getter = isIdentified) BOOL identified;

/**
 * Validates if the identified user is allowed to run this application. This checks
 * with the HockeyApp backend and calls the completion-block once completed.
 *
 * If the `BITAuthenticator` is in manual mode, it's your responsibility to call
 * this method. If the application is not yet identified, validation is not possible
 * and the completion-block is called with an error set.
 *
 * If the `BITAuthenticator` is in auto-mode, this is called by the authenticator itself
 * once needed.
 *
 * @see identificationType
 * @see authenticateInstallation
 * @see identifyWithCompletion:
 *
 * @param completion Block being executed once validation completed. Be sure to properly dispatch code to the main queue if necessary.
 */
- (void) validateWithCompletion:(void(^)(BOOL validated, NSError *error)) completion;

/**
 * Indicates if this installation is validated.
 */
@property (nonatomic, assign, readonly, getter = isValidated) BOOL validated;

/**
 * Removes all previously stored authentication tokens, UDIDs, etc.
 */
- (void) cleanupInternalStorage;

/**
 * Returns different values depending on `identificationType`. This can be used
 * by the application to identify the user.
 *
 * @see identificationType
 */
- (NSString*) publicInstallationIdentifier;
@end

#pragma mark - Protocol

/**
 * `BITAuthenticator` protocol
 */
@protocol BITAuthenticatorDelegate <NSObject>

@optional
/**
 * If the authentication (or validation) needs to identify the user,
 * this delegate method is called with the viewController that we'll present.
 *
 * @param authenticator `BITAuthenticator` object
 * @param viewController `UIViewController` used to identify the user
 *
 */
- (void) authenticator:(BITAuthenticator *)authenticator willShowAuthenticationController:(UIViewController*) viewController;
@end
