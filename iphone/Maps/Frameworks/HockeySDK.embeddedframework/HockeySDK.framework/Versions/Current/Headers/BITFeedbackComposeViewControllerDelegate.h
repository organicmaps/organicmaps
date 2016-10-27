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
 *  The users action when composing a message
 */
typedef NS_ENUM(NSUInteger, BITFeedbackComposeResult) {
  /**
   *  user hit cancel
   */
  BITFeedbackComposeResultCancelled,
  /**
   *  user hit submit
   */
  BITFeedbackComposeResultSubmitted,
};

@class BITFeedbackComposeViewController;

/**
 * The `BITFeedbackComposeViewControllerDelegate` formal protocol defines methods further configuring
 * the behaviour of `BITFeedbackComposeViewController`.
 */

@protocol BITFeedbackComposeViewControllerDelegate <NSObject>

@optional

///-----------------------------------------------------------------------------
/// @name View Controller Management
///-----------------------------------------------------------------------------

/**
 * Invoked once the compose screen is finished via send or cancel
 *
 * If this is implemented, it's the responsibility of this method to dismiss the presented
 * `BITFeedbackComposeViewController`
 *
 * @param composeViewController The `BITFeedbackComposeViewController` instance invoking this delegate
 * @param composeResult The user action the lead to closing the compose view
 */
- (void)feedbackComposeViewController:(BITFeedbackComposeViewController *)composeViewController
                  didFinishWithResult:(BITFeedbackComposeResult) composeResult;

#pragma mark - Deprecated methods

/**
 * This method is deprecated. If feedbackComposeViewController:didFinishWithResult: is implemented, this will not be called
 *
 * @param composeViewController The `BITFeedbackComposeViewController` instance invoking this delegate
 */
- (void)feedbackComposeViewControllerDidFinish:(BITFeedbackComposeViewController *)composeViewController __attribute__((deprecated("Use feedbackComposeViewController:didFinishWithResult: instead")));
@end
