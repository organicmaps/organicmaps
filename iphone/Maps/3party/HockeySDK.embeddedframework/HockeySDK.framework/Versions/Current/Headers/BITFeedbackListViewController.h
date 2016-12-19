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


#import <UIKit/UIKit.h>
#import <QuickLook/QuickLook.h>

#import "BITHockeyBaseViewController.h"

/**
 View controller providing a default interface to manage feedback

 The message list interface contains options to locally delete single messages
 by swiping over them, or deleting all messages. This will not delete the messages
 on the server though!

 It is also integrates actions to invoke the user interface to compose a new messages,
 reload the list content from the server and changing the users name or email if these
 are allowed to be set.

 To add this view controller to your own app and push it onto a navigation stack,
 don't create the instance yourself, but use the following code to get a correct instance:

     [[BITHockeyManager sharedHockeyManager].feedbackManager feedbackListViewController:NO]

 To show it modally, use the following code instead:

     [[BITHockeyManager sharedHockeyManager].feedbackManager feedbackListViewController:YES]

 This ensures that the presentation on iOS 6 and iOS 7 will use the current design on each OS Version.
 */

@interface BITFeedbackListViewController : BITHockeyBaseViewController <UITableViewDelegate, UITableViewDataSource, UIActionSheetDelegate, UIAlertViewDelegate, QLPreviewControllerDataSource> {
}

@end
