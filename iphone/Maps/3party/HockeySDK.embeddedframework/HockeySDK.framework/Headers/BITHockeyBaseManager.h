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
#import <UIKit/UIKit.h>


/**
 The internal superclass for all component managers

 */

@interface BITHockeyBaseManager : NSObject

///-----------------------------------------------------------------------------
/// @name Modules
///-----------------------------------------------------------------------------


/**
 Defines the server URL to send data to or request data from

 By default this is set to the HockeyApp servers and there rarely should be a
 need to modify that.
 */
@property (nonatomic, copy) NSString *serverURL;


///-----------------------------------------------------------------------------
/// @name User Interface
///-----------------------------------------------------------------------------

/**
 The UIBarStyle of the update user interface navigation bar.

 Default is UIBarStyleBlackOpaque
 @see navigationBarTintColor
 */
@property (nonatomic, assign) UIBarStyle barStyle;

/**
 The navigationbar tint color of the update user interface navigation bar.

 The navigationBarTintColor is used by default, you can either overwrite it `navigationBarTintColor`
 or define another `barStyle` instead.

 Default is RGB(25, 25, 25)
 @see barStyle
 */
@property (nonatomic, strong) UIColor *navigationBarTintColor;

/**
 The UIModalPresentationStyle for showing the update user interface when invoked
 with the update alert.
 */
@property (nonatomic, assign) UIModalPresentationStyle modalPresentationStyle;


@end
