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

#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

#import "../alohalytics_objc.h"
#include "../alohalytics.h"
#include "../logger.h"

#include <utility> // std::pair
#include <sys/xattr.h>
#include <TargetConditionals.h> // TARGET_OS_IPHONE

#import <CoreFoundation/CoreFoundation.h>
#import <CoreFoundation/CFURL.h>
#import <Foundation/NSURL.h>
#if (TARGET_OS_IPHONE > 0)  // Works for all iOS devices, including iPad.
#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIApplication.h>
#import <UiKit/UIWebView.h>
#import <AdSupport/ASIdentifierManager.h>
// Export user agent for HTTP module.
NSString * gBrowserUserAgent = nil;
#endif  // TARGET_OS_IPHONE

#import <sys/socket.h>
#import <netinet/in.h>
#import <SystemConfiguration/SystemConfiguration.h>

using namespace alohalytics;

namespace {
// Conversion from [possible nil] NSString to std::string.
static std::string ToStdString(NSString * nsString) {
  if (nsString) {
    return std::string([nsString UTF8String]);
  }
  return std::string();
}

// Additional check if object can be represented as a string.
static std::string ToStdStringSafe(id object) {
  if ([object isKindOfClass:[NSString class]]) {
    return ToStdString(object);
  } else if ([object isKindOfClass:[NSObject class]]) {
    return ToStdString(((NSObject *)object).description);
  }
  return "ERROR: Trying to log neither NSString nor NSObject-inherited object.";
}

// Safe conversion from [possible nil] NSDictionary.
static TStringMap ToStringMap(NSDictionary * nsDictionary) {
  TStringMap map;
  for (NSString * key in nsDictionary) {
    map[ToStdString(key)] = ToStdStringSafe([nsDictionary objectForKey:key]);
  }
  return map;
}

// Safe conversion from [possible nil] NSArray.
static TStringMap ToStringMap(NSArray * nsArray) {
  TStringMap map;
  std::string key;
  for (id item in nsArray) {
    if (key.empty()) {
      key = ToStdStringSafe(item);
      map[key] = "";
    } else {
      map[key] = ToStdStringSafe(item);
      key.clear();
    }
  }
  return map;
}

// Safe extraction from [possible nil] CLLocation to alohalytics::Location.
static Location ExtractLocation(CLLocation * l) {
  Location extracted;
  if (!l) {
    return extracted;
  }
  // Validity of values is checked according to Apple's documentation:
  // https://developer.apple.com/library/ios/documentation/CoreLocation/Reference/CLLocation_Class/
  if (l.horizontalAccuracy >= 0) {
    extracted.SetLatLon([l.timestamp timeIntervalSince1970] * 1000.,
                        l.coordinate.latitude, l.coordinate.longitude,
                        l.horizontalAccuracy);
  }
  if (l.verticalAccuracy >= 0) {
    extracted.SetAltitude(l.altitude, l.verticalAccuracy);
  }
  if (l.speed >= 0) {
    extracted.SetSpeed(l.speed);
  }
  if (l.course >= 0) {
    extracted.SetBearing(l.course);
  }
  // We don't know location source on iOS.
  return extracted;
}

// Returns string representing uint64_t timestamp of given file or directory (modification date in millis from 1970).
static std::string PathTimestampMillis(NSString * path) {
  NSDictionary * attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil];
  if (attributes) {
    NSDate * date = [attributes objectForKey:NSFileModificationDate];
    return std::to_string(static_cast<uint64_t>([date timeIntervalSince1970] * 1000.));
  }
  return std::string("0");
}

#if (TARGET_OS_IPHONE > 0)
static std::string RectToString(CGRect const & rect) {
  return std::to_string(static_cast<int>(rect.origin.x)) + " " + std::to_string(static_cast<int>(rect.origin.y)) + " "
  + std::to_string(static_cast<int>(rect.size.width)) + " " + std::to_string(static_cast<int>(rect.size.height));
}

// Logs some basic device's info.
static void LogSystemInformation() {
  UIDevice * device = [UIDevice currentDevice];
  UIScreen * screen = [UIScreen mainScreen];
  std::string preferredLanguages;
  for (NSString * lang in [NSLocale preferredLanguages]) {
    preferredLanguages += [lang UTF8String] + std::string(" ");
  }
  std::string preferredLocalizations;
  for (NSString * loc in [[NSBundle mainBundle] preferredLocalizations]) {
    preferredLocalizations += [loc UTF8String] + std::string(" ");
  }
  NSLocale * locale = [NSLocale currentLocale];
  std::string userInterfaceIdiom = "phone";
  if (device.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
    userInterfaceIdiom = "pad";
  } else if (device.userInterfaceIdiom == UIUserInterfaceIdiomUnspecified) {
    userInterfaceIdiom = "unspecified";
  }
  alohalytics::TStringMap info = {
    {"bundleIdentifier", ToStdString([[NSBundle mainBundle] bundleIdentifier])},
    {"deviceName", ToStdString(device.name)},
    {"deviceSystemName", ToStdString(device.systemName)},
    {"deviceSystemVersion", ToStdString(device.systemVersion)},
    {"deviceModel", ToStdString(device.model)},
    {"deviceUserInterfaceIdiom", userInterfaceIdiom},
    {"screens", std::to_string([UIScreen screens].count)},
    {"screenBounds", RectToString(screen.bounds)},
    {"screenScale", std::to_string(screen.scale)},
    {"preferredLanguages", preferredLanguages},
    {"preferredLocalizations", preferredLocalizations},
    {"localeIdentifier", ToStdString([locale objectForKey:NSLocaleIdentifier])},
    {"calendarIdentifier", ToStdString([[locale objectForKey:NSLocaleCalendar] calendarIdentifier])},
    {"localeMeasurementSystem", ToStdString([locale objectForKey:NSLocaleMeasurementSystem])},
    {"localeDecimalSeparator", ToStdString([locale objectForKey:NSLocaleDecimalSeparator])},
  };
  if (device.systemVersion.floatValue >= 8.0) {
    info.emplace("screenNativeBounds", RectToString(screen.nativeBounds));
    info.emplace("screenNativeScale", std::to_string(screen.nativeScale));
  }
  Stats & instance = Stats::Instance();
  instance.LogEvent("$iosDeviceInfo", info);

  info.clear();
  if (device.systemVersion.floatValue >= 6.0) {
    if (device.identifierForVendor) {
      info.emplace("identifierForVendor", ToStdString(device.identifierForVendor.UUIDString));
    }
    if (NSClassFromString(@"ASIdentifierManager")) {
      ASIdentifierManager * manager = [ASIdentifierManager sharedManager];
      info.emplace("isAdvertisingTrackingEnabled", manager.isAdvertisingTrackingEnabled ? "YES" : "NO");
      if (manager.advertisingIdentifier) {
        info.emplace("advertisingIdentifier", ToStdString(manager.advertisingIdentifier.UUIDString));
      }
    }
  }
  if (!info.empty()) {
    instance.LogEvent("$iosDeviceIds", info);
  }
}
#endif  // TARGET_OS_IPHONE

// Returns <unique id, true if it's the very-first app launch>.
static std::pair<std::string, bool> InstallationId() {
  bool firstLaunch = false;
  NSUserDefaults * userDataBase = [NSUserDefaults standardUserDefaults];
  NSString * installationId = [userDataBase objectForKey:@"AlohalyticsInstallationId"];
  if (installationId == nil) {
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    // All iOS IDs start with I:
    installationId = [@"I:" stringByAppendingString:(NSString *)CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid))];
    CFRelease(uuid);
    [userDataBase setValue:installationId forKey:@"AlohalyticsInstallationId"];
    [userDataBase synchronize];
    firstLaunch = true;
  }
  return std::make_pair([installationId UTF8String], firstLaunch);
}

// Returns path to store statistics files.
static std::string StoragePath() {
  // Store files in special directory which is not backed up automatically.
  NSArray * paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString * directory = [[paths firstObject] stringByAppendingString:@"/Alohalytics/"];
  NSFileManager * fm = [NSFileManager defaultManager];
  if (![fm fileExistsAtPath:directory]) {
    if (![fm createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil]) {
      // TODO(AlexZ): Probably we need to log this case to the server in the future.
      NSLog(@"Alohalytics ERROR: Can't create directory %@.", directory);
    }
#if (TARGET_OS_IPHONE > 0)
    // Disable iCloud backup for storage folder: https://developer.apple.com/library/iOS/qa/qa1719/_index.html
    const std::string storagePath = [directory UTF8String];
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                           reinterpret_cast<unsigned char const *>(storagePath.c_str()),
                                                           storagePath.size(),
                                                           0);
    CFErrorRef err;
    signed char valueOfCFBooleanYes = 1;
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberCharType, &valueOfCFBooleanYes);
    if (!CFURLSetResourcePropertyForKey(url, kCFURLIsExcludedFromBackupKey, value, &err)) {
      NSLog(@"Alohalytics ERROR while disabling iCloud backup for directory %@", directory);
    }
    CFRelease(value);
    CFRelease(url);
#endif  // TARGET_OS_IPHONE
  }
  if (directory) {
    return [directory UTF8String];
  }
  return std::string("Alohalytics ERROR: Can't retrieve valid storage path.");
}

#if (TARGET_OS_IPHONE > 0)
static alohalytics::TStringMap ParseLaunchOptions(NSDictionary * options) {
  TStringMap parsed;
  NSURL * url = [options objectForKey:UIApplicationLaunchOptionsURLKey];
  if (url) {
    parsed.emplace("UIApplicationLaunchOptionsURLKey", ToStdString([url absoluteString]));
  }
  NSString * source = [options objectForKey:UIApplicationLaunchOptionsSourceApplicationKey];
  if (source) {
    parsed.emplace("UIApplicationLaunchOptionsSourceApplicationKey", ToStdString(source));
  }
  return parsed;
}

// Need it to effectively upload data when app goes into background.
static UIBackgroundTaskIdentifier sBackgroundTaskId = UIBackgroundTaskInvalid;
static void EndBackgroundTask() {
  [[UIApplication sharedApplication] endBackgroundTask:sBackgroundTaskId];
  sBackgroundTaskId = UIBackgroundTaskInvalid;
}
static void OnUploadFinished(alohalytics::ProcessingResult result) {
  if (Stats::Instance().DebugMode()) {
    const char * str;
    switch (result) {
      case alohalytics::ProcessingResult::ENothingToProcess: str = "There is no data to upload."; break;
      case alohalytics::ProcessingResult::EProcessedSuccessfully: str = "Data was uploaded successfully."; break;
      case alohalytics::ProcessingResult::EProcessingError: str = "Error while uploading data."; break;
    }
    ALOG(str);
  }
  if (sBackgroundTaskId != UIBackgroundTaskInvalid) {
    EndBackgroundTask();
  }
}

// Quick check if device has any active connection.
// Does not guarantee actual reachability of any host.
bool IsConnectionActive() {
  struct sockaddr_in zero;
  bzero(&zero, sizeof(zero));
  zero.sin_len = sizeof(zero);
  zero.sin_family = AF_INET;
  SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zero);
  if (!reachability) {
    return false;
  }
  SCNetworkReachabilityFlags flags;
  const bool gotFlags = SCNetworkReachabilityGetFlags(reachability, &flags);
  CFRelease(reachability);
  if (!gotFlags || ((flags & kSCNetworkReachabilityFlagsReachable) == 0)) {
    return false;
  }
  if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
    return true;
  }
  if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) || (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)
      && (flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
    return true;
  }
  if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN) {
    return true;
  }
  return false;
}

#endif  // TARGET_OS_IPHONE
} // namespace

@implementation Alohalytics

+ (void)setDebugMode:(BOOL)enable {
  Stats::Instance().SetDebugMode(enable);
}

+ (void)setup:(NSString *)serverUrl withLaunchOptions:(NSDictionary *)options {
  [Alohalytics setup:serverUrl andFirstLaunch:YES withLaunchOptions:options];
}

+ (void)setup:(NSString *)serverUrl andFirstLaunch:(BOOL)isFirstLaunch withLaunchOptions:(NSDictionary *)options {
#if (TARGET_OS_IPHONE > 0)
  // Initialize User Agent later, as it takes significant time at startup.
  dispatch_async(dispatch_get_main_queue(), ^(void) {
    gBrowserUserAgent = [[[UIWebView alloc] initWithFrame:CGRectZero] stringByEvaluatingJavaScriptFromString:@"navigator.userAgent"];
    if (gBrowserUserAgent) {
      Stats::Instance().LogEvent("$browserUserAgent", ToStdString(gBrowserUserAgent));
    }
  });
  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  Class cls = [Alohalytics class];
  [nc addObserver:cls selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
  [nc addObserver:cls selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
  [nc addObserver:cls selector:@selector(applicationWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
  [nc addObserver:cls selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
  [nc addObserver:cls selector:@selector(applicationWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];
#endif // TARGET_OS_IPHONE
  const auto installationId = InstallationId();
  Stats & instance = Stats::Instance();
  instance.SetClientId(installationId.first)
          .SetServerUrl([serverUrl UTF8String])
          .SetStoragePath(StoragePath());

  // Calculate some basic statistics about installations/updates/launches.
  NSUserDefaults * userDataBase = [NSUserDefaults standardUserDefaults];
  NSString * installedVersion = [userDataBase objectForKey:@"AlohalyticsInstalledVersion"];
  NSBundle * bundle = [NSBundle mainBundle];
  if (installationId.second && isFirstLaunch && installedVersion == nil) {
    NSString * version = [[bundle infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    // Documents folder modification time can be interpreted as a "first app launch time" or an approx. "app install time".
    // App bundle modification time can be interpreted as an "app update time".
    instance.LogEvent("$install", {{"CFBundleShortVersionString", [version UTF8String]},
        {"documentsTimestampMillis", PathTimestampMillis([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject])},
        {"bundleTimestampMillis", PathTimestampMillis([bundle executablePath])}});
    [userDataBase setValue:version forKey:@"AlohalyticsInstalledVersion"];
    [userDataBase synchronize];
#if (TARGET_OS_IPHONE > 0)
    LogSystemInformation();
#else
    static_cast<void>(options);  // Unused variable warning fix.
#endif  // TARGET_OS_IPHONE
  } else {
    NSString * version = [[bundle infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    if (installedVersion == nil || ![installedVersion isEqualToString:version]) {
      instance.LogEvent("$update", {{"CFBundleShortVersionString", [version UTF8String]},
          {"documentsTimestampMillis", PathTimestampMillis([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject])},
          {"bundleTimestampMillis", PathTimestampMillis([bundle executablePath])}});
      [userDataBase setValue:version forKey:@"AlohalyticsInstalledVersion"];
      [userDataBase synchronize];
#if (TARGET_OS_IPHONE > 0)
      LogSystemInformation();
#endif  // TARGET_OS_IPHONE
    }
  }
  instance.LogEvent("$launch"
#if (TARGET_OS_IPHONE > 0)
                    , ParseLaunchOptions(options)
#endif  // TARGET_OS_IPHONE
                    );
}

+ (void)forceUpload {
  Stats::Instance().Upload();
}

+ (void)logEvent:(NSString *)event {
  Stats::Instance().LogEvent(ToStdString(event));
}

+ (void)logEvent:(NSString *)event atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withValue:(NSString *)value {
  Stats::Instance().LogEvent(ToStdString(event), ToStdString(value));
}

+ (void)logEvent:(NSString *)event withValue:(NSString *)value atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStdString(value), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withKeyValueArray:(NSArray *)array {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(array));
}

+ (void)logEvent:(NSString *)event withKeyValueArray:(NSArray *)array atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(array), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withDictionary:(NSDictionary *)dictionary {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(dictionary));
}

+ (void)logEvent:(NSString *)event withDictionary:(NSDictionary *)dictionary atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(dictionary), ExtractLocation(location));
}

#pragma mark App lifecycle notifications used to calculate basic metrics.
#if (TARGET_OS_IPHONE > 0)
+ (void)applicationDidBecomeActive:(NSNotification *)notification {
  Stats::Instance().LogEvent("$applicationDidBecomeActive");
}

+ (void)applicationWillResignActive:(NSNotification *)notification {
  Stats::Instance().LogEvent("$applicationWillResignActive");
}

+ (void)applicationWillEnterForeground:(NSNotificationCenter *)notification {
  Stats::Instance().LogEvent("$applicationWillEnterForeground");

  if (sBackgroundTaskId != UIBackgroundTaskInvalid) {
    EndBackgroundTask();
  }
}

+ (void)applicationDidEnterBackground:(NSNotification *)notification {
  Stats::Instance().LogEvent("$applicationDidEnterBackground");

  sBackgroundTaskId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{ EndBackgroundTask(); }];
  if (IsConnectionActive()) {
    // Start uploading in the background, we have about 10 mins to do that.
    alohalytics::Stats::Instance().Upload(&OnUploadFinished);
  } else {
    if (Stats::Instance().DebugMode()) {
      ALOG("Skipped statistics uploading as connection is not active.");
    }
  }
}

+ (void)applicationWillTerminate:(NSNotification *)notification {
  Stats::Instance().LogEvent("$applicationWillTerminate");
}
#endif // TARGET_OS_IPHONE
@end
