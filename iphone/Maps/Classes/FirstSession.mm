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

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

#import "FirstSession.h"

#include <sys/xattr.h>
#include <utility>  // std::pair

#import <CoreFoundation/CFURL.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/NSURL.h>
#if (TARGET_OS_IPHONE > 0)  // Works for all iOS devices, including iPad.
#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIWebView.h>
#endif  // TARGET_OS_IPHONE

#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>
#import <sys/socket.h>

namespace
{
// Key for app unique installation id in standardUserDefaults.
NSString * const kAlohalyticsInstallationId = @"AlohalyticsInstallationId";
}  // namespace

// Keys for NSUserDefaults.
static NSString * const kInstalledVersionKey = @"AlohalyticsInstalledVersion";
static NSString * const kFirstLaunchDateKey = @"AlohalyticsFirstLaunchDate";
static NSString * const kTotalSecondsInTheApp = @"AlohalyticsTotalSecondsInTheApp";
static NSString * const kIsAlohalyticsDisabledKey = @"AlohalyticsDisabledKey";

// Used to calculate session length and total time spent in the app.
// setup should be called to activate counting.
static NSDate * gSessionStartTime = nil;
static BOOL gIsFirstSession = NO;
static NSString * gInstallationId = nil;

@implementation FirstSession

+ (void)setup:(NSArray *)serverUrls withLaunchOptions:(NSDictionary *)options
{
#if (TARGET_OS_IPHONE > 0)
  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  Class cls = [FirstSession class];
  [nc addObserver:cls
         selector:@selector(applicationDidEnterBackground:)
             name:UIApplicationDidEnterBackgroundNotification
           object:nil];
  [nc addObserver:cls
         selector:@selector(applicationWillTerminate:)
             name:UIApplicationWillTerminateNotification
           object:nil];
#endif  // TARGET_OS_IPHONE
  // INIT
  [self installationId];
}
#if (TARGET_OS_IPHONE > 0)
+ (void)applicationDidEnterBackground:(NSNotification *)notification
{
  if (gIsFirstSession)
    gIsFirstSession = NO;
}

+ (void)applicationWillTerminate:(NSNotification *)notification
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}
#endif  // TARGET_OS_IPHONE

#pragma mark Utility methods

+ (BOOL)isFirstSession
{
  return [self installationId] != nil && gIsFirstSession;
}

+ (NSDate *)firstLaunchDate
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * date = [ud objectForKey:kFirstLaunchDateKey];
  if (!date)
  {
    // Non-standard situation: this method is called before calling setup. Return current date.
    date = [NSDate date];
    [ud setObject:date forKey:kFirstLaunchDateKey];
  }
  return date;
}

+ (NSInteger)totalSecondsSpentInTheApp
{
  NSInteger seconds = [[NSUserDefaults standardUserDefaults] integerForKey:kTotalSecondsInTheApp];
  // Take into an account currently active session.
  if (gSessionStartTime)
    seconds += static_cast<NSInteger>(-gSessionStartTime.timeIntervalSinceNow);
  return seconds;
}

// Internal helper, returns nil for invalid paths.
+ (NSDate *)fileCreationDate:(NSString *)fullPath
{
  NSDictionary * attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:fullPath error:nil];
  return attributes ? [attributes objectForKey:NSFileCreationDate] : nil;
}

+ (NSDate *)installDate
{
  return [FirstSession
      fileCreationDate:[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject]];
}

+ (NSDate *)updateDate
{
  return [FirstSession fileCreationDate:[[NSBundle mainBundle] resourcePath]];
}

+ (NSDate *)buildDate
{
  return [FirstSession fileCreationDate:[[NSBundle mainBundle] executablePath]];
}

+ (NSString *)installationId
{
  if (gInstallationId == nil)
  {
    gIsFirstSession = NO;
    NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
    gInstallationId = [ud stringForKey:kAlohalyticsInstallationId];
    if (gInstallationId == nil)
    {
      CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
      // All iOS IDs start with I:
      gInstallationId =
          [@"I:" stringByAppendingString:(NSString *)CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid))];
      CFRelease(uuid);
      [ud setValue:gInstallationId forKey:kAlohalyticsInstallationId];
      gIsFirstSession = YES;
    }
  }
  return gInstallationId;
}

@end
