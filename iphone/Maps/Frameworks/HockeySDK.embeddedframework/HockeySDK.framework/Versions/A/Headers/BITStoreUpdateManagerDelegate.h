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

#import <Foundation/Foundation.h>

@class BITStoreUpdateManager;

/**
 The `BITStoreUpdateManagerDelegate` formal protocol defines methods for
 more interaction with `BITStoreUpdateManager`.
 */

@protocol BITStoreUpdateManagerDelegate <NSObject>

@optional


///-----------------------------------------------------------------------------
/// @name Update information
///-----------------------------------------------------------------------------

/** Informs which new version has been reported to be available

 @warning If this is invoked with a simulated new version, the storeURL could be _NIL_ if the current builds
 bundle identifier is different to the bundle identifier used in the app store build.
 @param storeUpdateManager The `BITStoreUpdateManager` instance invoking this delegate
 @param newVersion The new version string reported by the App Store
 @param storeURL The App Store URL for this app that could be invoked to let them perform the update.
 */
-(void)detectedUpdateFromStoreUpdateManager:(BITStoreUpdateManager *)storeUpdateManager newVersion:(NSString *)newVersion storeURL:(NSURL *)storeURL;



@end
