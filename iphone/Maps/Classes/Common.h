#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

static NSString * const FIRST_LAUNCH_KEY = @"FIRST_LAUNCH_KEY";

static NSString * const MAPSWITHME_PREMIUM_LOCAL_URL = @"mapswithmepro://";

static NSString * const BOOKMARK_CATEGORY_DELETED_NOTIFICATION = @"BookmarkCategoryDeletedNotification";

static NSString * const METRICS_CHANGED_NOTIFICATION = @"MetricsChangedNotification";

static NSString * const BOOKMARK_DELETED_NOTIFICATION = @"BookmarkDeletedNotification";

static NSString * const LOCATION_MANAGER_STARTED_NOTIFICATION = @"LocationManagerStartedNotification";

static NSString * const kDownloadingProgressUpdateNotificationId = @"DownloadingProgressUpdateNotificationId";

static NSString * const kHaveAppleWatch = @"HaveAppleWatch";

static inline NSString * const kApplicationGroupIdentifier()
{
  static NSString * const productionGroupIdentifier = @"group.mapsme.watchkit.production";
  static NSString * const developerGroupIdentifier = @"group.mapsme.watchkit";

  static NSString * const productionAppBundleIdentifier = @"com.mapswithme.full";
  static NSString * const productionExtBundleIdentifier = @"com.mapswithme.full.watchkitextension";

  NSString * bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
  if ([bundleIdentifier isEqualToString:productionAppBundleIdentifier] || [bundleIdentifier isEqualToString:productionExtBundleIdentifier])
    return productionGroupIdentifier;
  return developerGroupIdentifier;
}

static inline BOOL firstVersionIsLessThanSecond(NSString * first, NSString * second)
{
  NSArray const * const f = [first componentsSeparatedByString:@"."];
  NSArray const * const s = [second componentsSeparatedByString:@"."];
  NSUInteger iter = 0;
  while (f.count > iter && s.count > iter)
  {
    NSInteger fiv = ((NSString *)f[iter]).integerValue;
    NSInteger siv = ((NSString *)s[iter]).integerValue;

    if (fiv == siv)
      iter++;
    else
      return fiv < siv;
  }
  return f.count < s.count;
}

static inline BOOL isIOSVersionLessThan(NSString * version)
{
  return firstVersionIsLessThanSecond([UIDevice currentDevice].systemVersion, version);
}

static inline BOOL isIOSVersionLessThan(NSUInteger version)
{
  return isIOSVersionLessThan([NSString stringWithFormat:@"%@", @(version)]);
}
