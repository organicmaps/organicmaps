#import "MWMPushNotifications.h"

#include "platform/marketing_service.hpp"
#include "platform/platform.hpp"


// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.

namespace
{
NSString * const kPushDeviceTokenLogEvent = @"iOSPushDeviceToken";
}  // namespace

@implementation MWMPushNotifications

+ (void)setup
{
}

+ (void)application:(UIApplication *)application
    didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
}

+ (void)application:(UIApplication *)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
}

+ (void)application:(UIApplication *)application
    didReceiveRemoteNotification:(NSDictionary *)userInfo
          fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  completionHandler(UIBackgroundFetchResultNoData);
}

+ (BOOL)handleURLPush:(NSDictionary *)userInfo
{
  auto app = UIApplication.sharedApplication;
  if (app.applicationState != UIApplicationStateInactive)
    return NO;

  NSString * openLink = userInfo[@"openURL"];
  if (!openLink)
    return NO;

  NSURL * url = [NSURL URLWithString:openLink];
  [app openURL:url options:@{} completionHandler:nil];
  return YES;
}

+ (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
}

+ (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void(^)(void))completionHandler
{
}

+ (NSString * _Nonnull)formattedTimestamp {
  return @(GetPlatform().GetMarketingService().GetPushWooshTimestamp().c_str());
}

@end
