/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2021 Alexander Borsuk <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

#ifndef FIRSTSESSION_H
#define FIRSTSESSION_H

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSDate.h>
#import <TargetConditionals.h> // TARGET_OS_IPHONE

#if (TARGET_OS_IPHONE > 0)
#import <UIKit/UIApplication.h> // enum UIBackgroundFetchResult
#endif // TARGET_OS_IPHONE

@interface FirstSession : NSObject
// Should be called in application:didFinishLaunchingWithOptions: or in application:willFinishLaunchingWithOptions:
// Final serverUrl is modified to $(serverUrl)/[ios|mac]/your.bundle.id/app.version
+ (void)setup:(NSArray *)serverUrls withLaunchOptions:(NSDictionary *)options;
// Returns YES if it is a first session, before app goes into background.
+ (BOOL)isFirstSession;
// Returns summary time of all active user sessions up to now.
+ (NSInteger)totalSecondsSpentInTheApp;

// Returns the date when app was launched for the first time (usually the same as install date).
+ (NSDate *)firstLaunchDate;
// When app was installed (it's Documents folder creation date).
// Note: firstLaunchDate is usually later than installDate.
+ (NSDate *)installDate;
// When app was updated (~== installDate for the first installation, it's Resources folder creation date).
+ (NSDate *)updateDate;
// When the binary was built.
// Hint: if buildDate > installDate then this is not a new app install, but an existing old user.
+ (NSDate *)buildDate;
// Returns app unique installation id or nil if called before setup in the first session.
+ (NSString *)installationId;
@end

#endif  // #ifndef FIRSTSESSION_H
