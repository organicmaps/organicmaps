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

#import "BITHockeyBaseManager.h"
#import "BITFeedbackListViewController.h"
#import "BITFeedbackComposeViewController.h"


// Notification message which tells that loading messages finished
#define BITHockeyFeedbackMessagesLoadingStarted @"BITHockeyFeedbackMessagesLoadingStarted"

// Notification message which tells that loading messages finished
#define BITHockeyFeedbackMessagesLoadingFinished @"BITHockeyFeedbackMessagesLoadingFinished"


/**
 *  Defines behavior of the user data field
 */
typedef NS_ENUM(NSInteger, BITFeedbackUserDataElement) {
  /**
   *  don't ask for this user data element at all
   */
  BITFeedbackUserDataElementDontShow = 0,
  /**
   *  the user may provide it, but does not have to
   */
  BITFeedbackUserDataElementOptional = 1,
  /**
   *  the user has to provide this to continue
   */
  BITFeedbackUserDataElementRequired = 2
};

/**
 *  Available modes for opening the feedback compose interface with a screenshot attached
 */
typedef NS_ENUM(NSInteger, BITFeedbackObservationMode) {
  /**
   *  No SDK provided trigger is active.
   */
  BITFeedbackObservationNone = 0,
  /**
   *  Triggers when the user takes a screenshot. This will grab the latest image from the camera roll. Requires iOS 7 or later!
   */
  BITFeedbackObservationModeOnScreenshot = 1,
  /**
   *  Triggers when the user taps with three fingers on the screen.
   */
  BITFeedbackObservationModeThreeFingerTap = 2
};


@class BITFeedbackMessage;
@protocol BITFeedbackManagerDelegate;

/**
 The feedback module.

 This is the HockeySDK module for letting your users communicate directly with you via
 the app and an integrated user interface. It provides a single threaded
 discussion with a user running your app.

 You should never create your own instance of `BITFeedbackManager` but use the one provided
 by the `[BITHockeyManager sharedHockeyManager]`:

     [BITHockeyManager sharedHockeyManager].feedbackManager

 The user interface provides a list view that can be presented modally using
 `[BITFeedbackManager showFeedbackListView]` or adding
 `[BITFeedbackManager feedbackListViewController:]` to push onto a navigation stack.
 This list integrates all features to load new messages, write new messages, view messages
 and ask the user for additional (optional) data like name and email.

 If the user provides the email address, all responses from the server will also be sent
 to the user via email and the user is also able to respond directly via email, too.

 The message list interface also contains options to locally delete single messages
 by swiping over them, or deleting all messages. This will not delete the messages
 on the server, though!

 It also integrates actions to invoke the user interface to compose a new message,
 reload the list content from the server and change the users name or email if these
 are allowed to be set.

 It is also possible to invoke the user interface to compose a new message in your
 own code, by calling `[BITFeedbackManager showFeedbackComposeView]` modally or adding
 `[BITFeedbackManager feedbackComposeViewController]` to push onto a navigation stack.

 If new messages are written while the device is offline, the SDK automatically retries to
 send them once the app starts again or gets active again, or if the notification
 `BITHockeyNetworkDidBecomeReachableNotification` is fired.

 A third option is to include the `BITFeedbackActivity` into an UIActivityViewController.
 This can be useful if you present some data that users can not only share but also
 report back to the developer because they have some problems, e.g. webcams not working
 any more. The activity provides a default title and image that can also be customized.

 New messages are automatically loaded on startup, when the app becomes active again
 or when the notification `BITHockeyNetworkDidBecomeReachableNotification` is fired. This
 only happens if the user ever did initiate a conversation by writing the first
 feedback message. The app developer has to fire this notification to trigger another retry
 when it detects the device having network access again. The SDK only retries automatically
 when the app becomes active again.

 Implementing the `BITFeedbackManagerDelegate` protocol will notify your app when a new
 message was received from the server. The `BITFeedbackComposeViewControllerDelegate`
 protocol informs your app about events related to sending feedback messages.

 */

@interface BITFeedbackManager : BITHockeyBaseManager

///-----------------------------------------------------------------------------
/// @name General settings
///-----------------------------------------------------------------------------


/**
 Define if a name has to be provided by the user when providing feedback

 - `BITFeedbackUserDataElementDontShow`: Don't ask for this user data element at all
 - `BITFeedbackUserDataElementOptional`: The user may provide it, but does not have to
 - `BITFeedbackUserDataElementRequired`: The user has to provide this to continue

 The default value is `BITFeedbackUserDataElementOptional`.

 @warning If you provide a non nil value for the `BITFeedbackManager` class via
 `[BITHockeyManagerDelegate userNameForHockeyManager:componentManager:]` then this
 property will automatically be set to `BITFeedbackUserDataElementDontShow`

 @see BITFeedbackUserDataElement
 @see requireUserEmail
 @see `[BITHockeyManagerDelegate userNameForHockeyManager:componentManager:]`
 */
@property (nonatomic, readwrite) BITFeedbackUserDataElement requireUserName;


/**
 Define if an email address has to be provided by the user when providing feedback

 If the user provides the email address, all responses from the server will also be send
 to the user via email and the user is also able to respond directly via email too.

 - `BITFeedbackUserDataElementDontShow`: Don't ask for this user data element at all
 - `BITFeedbackUserDataElementOptional`: The user may provide it, but does not have to
 - `BITFeedbackUserDataElementRequired`: The user has to provide this to continue

 The default value is `BITFeedbackUserDataElementOptional`.

 @warning If you provide a non nil value for the `BITFeedbackManager` class via
 `[BITHockeyManagerDelegate userEmailForHockeyManager:componentManager:]` then this
 property will automatically be set to `BITFeedbackUserDataElementDontShow`

 @see BITFeedbackUserDataElement
 @see requireUserName
 @see `[BITHockeyManagerDelegate userEmailForHockeyManager:componentManager:]`
 */
@property (nonatomic, readwrite) BITFeedbackUserDataElement requireUserEmail;


/**
 Indicates if an alert should be shown when new messages have arrived

 This lets the user view the new feedback by choosing the appropriate option
 in the alert sheet, and the `BITFeedbackListViewController` will be shown.

 The alert is only shown, if the newest message didn't originate from the current user.
 This requires the users email address to be present! The optional userid property
 cannot be used, because users could also answer via email and then this information
 is not available.

 Default is `YES`
 @see feedbackListViewController:
 @see requireUserEmail
 @see `[BITHockeyManagerDelegate userEmailForHockeyManager:componentManager:]`
 */
@property (nonatomic, readwrite) BOOL showAlertOnIncomingMessages;


/**
 Define the trigger that opens the feedback composer and attaches a screenshot

 The following modes are available:

 - `BITFeedbackObservationNone`: No SDK based trigger is active. You can implement your
   own trigger and then call `[[BITHockeyManager sharedHockeyManager].feedbackManager showFeedbackComposeViewWithGeneratedScreenshot];` to handle your custom events
   that should trigger this.
 - `BITFeedbackObservationModeOnScreenshot`: Triggers when the user takes a screenshot.
    This will grab the latest image from the camera roll. Requires iOS 7 or later!
 - `BITFeedbackObservationModeThreeFingerTap`: Triggers when the user taps on the screen for three seconds with three fingers.

 Default is `BITFeedbackObservationNone`

 @see showFeedbackComposeViewWithGeneratedScreenshot
 */
@property (nonatomic, readwrite) BITFeedbackObservationMode feedbackObservationMode;


/**
 Prefill feedback compose message user interface with the items given.

 All NSString-Content in the array will be concatenated and result in the message,
 while all UIImage and NSData-instances will be turned into attachments.

 @see `[BITFeedbackComposeViewController prepareWithItems:]`
 */
@property (nonatomic, copy) NSArray *feedbackComposerPreparedItems;


/**
 Don't show the option to add images from the photo library

 This is helpful if your application is landscape only, since the system UI for
 selecting an image from the photo library is portrait only

 This setting is used for all feedback compose views that are created by the
 `BITFeedbackManager`. If you invoke your own `BITFeedbackComposeViewController`,
 then set the appropriate property on the view controller directl!.
 */
@property (nonatomic) BOOL feedbackComposeHideImageAttachmentButton;


///-----------------------------------------------------------------------------
/// @name User Interface
///-----------------------------------------------------------------------------


/**
 Indicates if a forced user data UI presentation is shown modal

 If `requireUserName` and/or `requireUserEmail` are enabled, the first presentation
 of `feedbackListViewController:` and subsequent `feedbackComposeViewController:`
 will automatically present a UI that lets the user provide this data and compose
 a message. By default this is shown (since SDK 3.1) as a modal sheet.

 If you want the SDK to push this UI onto the navigation stack in this specific scenario,
 then change the property to `NO`.

 @warning If you are presenting the `BITFeedbackListViewController` in a popover, this property should not be changed!

 Default is `YES`
 @see requireUserName
 @see requireUserEmail
 @see showFeedbackComposeView
 @see feedbackComposeViewController
 @see showFeedbackListView
 @see feedbackListViewController:
 */
@property (nonatomic, readwrite) BOOL showFirstRequiredPresentationModal;


/**
 Return a screenshot UIImage instance from the current visible screen

 @return UIImage instance containing a screenshot of the current screen
 */
- (UIImage *)screenshot;


/**
 Present the modal feedback list user interface.

 @warning This methods needs to be called on the main thread!
 */
- (void)showFeedbackListView;


/**
 Create an feedback list view

 @param modal Return a view ready for modal presentation with integrated navigation bar
 @return `BITFeedbackListViewController` The feedback list view controller,
 e.g. to push it onto a navigation stack.
 */
- (BITFeedbackListViewController *)feedbackListViewController:(BOOL)modal;


/**
 Present the modal feedback compose message user interface.

 @warning This methods needs to be called on the main thread!
 */
- (void)showFeedbackComposeView;

/**
 Present the modal feedback compose message user interface with the items given.

 All NSString-Content in the array will be concatenated and result in the message,
 while all UIImage and NSData-instances will be turned into attachments.

 @param items an NSArray with objects that should be attached
 @see `[BITFeedbackComposeViewController prepareWithItems:]`
 @warning This methods needs to be called on the main thread!
 */
- (void)showFeedbackComposeViewWithPreparedItems:(NSArray *)items;

/**
 Presents a modal feedback compose interface with a screenshot attached which is taken at the time of calling this method.

 This should be used when your own trigger fires. The following code should be used:

     [[BITHockeyManager sharedHockeyManager].feedbackManager showFeedbackComposeViewWithGeneratedScreenshot];

 @see feedbackObservationMode
 @warning This methods needs to be called on the main thread!
 */
- (void)showFeedbackComposeViewWithGeneratedScreenshot;


/**
 Create a feedback compose view

 Example to show a modal feedback compose UI with prefilled text

     BITFeedbackComposeViewController *feedbackCompose = [[BITHockeyManager sharedHockeyManager].feedbackManager feedbackComposeViewController];

     [feedbackCompose prepareWithItems:
         @[@"Adding some example default text and also adding a link.",
         [NSURL URLWithString:@"http://hockeayyp.net/"]]];

     UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:feedbackCompose];
     navController.modalPresentationStyle = UIModalPresentationFormSheet;
     [self presentViewController:navController animated:YES completion:nil];

 @return `BITFeedbackComposeViewController` The compose feedback view controller,
 e.g. to push it onto a navigation stack.
 */
- (BITFeedbackComposeViewController *)feedbackComposeViewController;


@end
