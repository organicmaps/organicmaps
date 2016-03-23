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
 UIActivity subclass allowing to use the feedback interface to share content with the developer

 This activity can be added into an UIActivityViewController and it will use the activity data
 objects to prefill the content of `BITFeedbackComposeViewController`.

 This can be useful if you present some data that users can not only share but also
 report back to the developer because they have some problems, e.g. webcams not working
 any more.

 The activity provide a default title and image that can be further customized
 via `customActivityImage` and `customActivityTitle`.

 */

@interface BITFeedbackActivity : UIActivity <BITFeedbackComposeViewControllerDelegate>

///-----------------------------------------------------------------------------
/// @name BITFeedbackActivity customisation
///-----------------------------------------------------------------------------


/**
 Define the image shown when using `BITFeedbackActivity`

 If not set a default icon is being used.

 @see customActivityTitle
 */
@property (nonatomic, strong) UIImage *customActivityImage;


/**
 Define the title shown when using `BITFeedbackActivity`

 If not set, a default string is shown by using the apps name
 and adding the localized string "Feedback" to it.

 @see customActivityImage
 */
@property (nonatomic, strong) NSString *customActivityTitle;

@end
