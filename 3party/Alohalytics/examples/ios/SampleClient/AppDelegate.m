/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

#import "AppDelegate.h"

#import "../../../src/alohalytics_objc.h"

@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  // Override point for customization after application launch.
  [Alohalytics setDebugMode:YES];
  [Alohalytics setup:@"http://localhost:8080/" withLaunchOptions:launchOptions];

  UIDevice * device = [UIDevice currentDevice];
  [Alohalytics logEvent:@"deviceInfo" withKeyValueArray:@[@"deviceName", device.name, @"systemName", device.systemName, @"systemVersion", device.systemVersion]];

  // Used for example purposes only to upload statistics (unpredictable) in background, when system wakes app up.
  [application setMinimumBackgroundFetchInterval:UIApplicationBackgroundFetchIntervalMinimum];

  return YES;
}

// This is an additional/optional way to upload statistics on the server after a long offline usage.
// You have to set "fetch" in UIBackgroundsModes in your plist so system will call this method.
// See https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/iPhoneOSKeys.html for more details.
-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  // TODO(AlexZ): This method should be synchronous.
  [Alohalytics forceUpload];
  completionHandler(UIBackgroundFetchResultNewData);
}

@end
