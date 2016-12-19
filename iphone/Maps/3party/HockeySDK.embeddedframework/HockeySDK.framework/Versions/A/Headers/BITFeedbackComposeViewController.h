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

#import "BITFeedbackComposeViewControllerDelegate.h"

/**
 View controller allowing the user to write and send new feedback

 To add this view controller to your own app and push it onto a navigation stack,
 don't create the instance yourself, but use the following code to get a correct instance:

     [[BITHockeyManager sharedHockeyManager].feedbackManager feedbackComposeViewController]

 To show it modally, use the following code instead:

     [[BITHockeyManager sharedHockeyManager].feedbackManager showFeedbackComposeView]

 */

@interface BITFeedbackComposeViewController : UIViewController <UITextViewDelegate>


///-----------------------------------------------------------------------------
/// @name Delegate
///-----------------------------------------------------------------------------


/**
 Sets the `BITFeedbackComposeViewControllerDelegate` delegate.

 The delegate is automatically set by using `[BITHockeyManager setDelegate:]`. You
 should not need to set this delegate individually.

 @see `[BITHockeyManager setDelegate:`]
 */
@property (nonatomic, weak) id<BITFeedbackComposeViewControllerDelegate> delegate;


///-----------------------------------------------------------------------------
/// @name Presetting content
///-----------------------------------------------------------------------------


/**
 Don't show the option to add images from the photo library

 This is helpful if your application is landscape only, since the system UI for
 selecting an image from the photo library is portrait only
 */
@property (nonatomic) BOOL hideImageAttachmentButton;

/**
 An array of data objects that should be used to prefill the compose view content

 The following data object classes are currently supported:
 - NSString
 - NSURL
 - UIImage
 - NSData
 - `BITHockeyAttachment`

 These are automatically concatenated to one text string, while any images and NSData
 objects are added as attachments to the feedback.

 @param items Array of data objects to prefill the feedback text message.
 */
- (void)prepareWithItems:(NSArray *)items;

@end
