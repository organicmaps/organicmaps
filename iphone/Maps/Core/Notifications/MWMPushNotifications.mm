#import "MWMPushNotifications.h"
#import <Crashlytics/Crashlytics.h>
#import <Pushwoosh/PushNotificationManager.h>
#import <UserNotifications/UserNotifications.h>
#import "MWMCommon.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.
#import "private.h"

#include "std/string.hpp"

namespace
{
NSString * const kPushDeviceTokenLogEvent = @"iOSPushDeviceToken";
}  // namespace

@implementation MWMPushNotifications

+ (void)setup:(NSDictionary *)launchOptions
{
  PushNotificationManager * pushManager = [PushNotificationManager pushManager];

  if (!isIOSVersionLessThan(10))
    [UNUserNotificationCenter currentNotificationCenter].delegate = pushManager.notificationCenterDelegate;

  [pushManager handlePushReceived:launchOptions];

  // make sure we count app open in Pushwoosh stats
  [pushManager sendAppOpen];

  // register for push notifications!
  [pushManager registerForPushNotifications];
}

+ (void)application:(UIApplication *)application
    didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
  PushNotificationManager * pushManager = [PushNotificationManager pushManager];
  [pushManager handlePushRegistration:deviceToken];
  [Alohalytics logEvent:kPushDeviceTokenLogEvent withValue:pushManager.getHWID];
}

+ (void)application:(UIApplication *)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
  [[PushNotificationManager pushManager] handlePushRegistrationFailure:error];
}

+ (void)application:(UIApplication *)application
    didReceiveRemoteNotification:(NSDictionary *)userInfo
          fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  [Statistics logEvent:kStatEventName(kStatApplication, kStatPushReceived) withParameters:userInfo];
  if (![self handleURLPush:userInfo])
    [[PushNotificationManager pushManager] handlePushReceived:userInfo];
  completionHandler(UIBackgroundFetchResultNoData);
}

+ (BOOL)handleURLPush:(NSDictionary *)userInfo
{
  auto app = UIApplication.sharedApplication;
  CLS_LOG(@"Handle url push");
  CLS_LOG(@"User info: %@", userInfo);
  if (app.applicationState != UIApplicationStateInactive)
    return NO;
  NSString * openLink = userInfo[@"openURL"];
  CLS_LOG(@"Push's url: %@", openLink);
  if (!openLink)
    return NO;
  NSURL * url = [NSURL URLWithString:openLink];
  [app openURL:url];
  return YES;
}

+ (NSString *)pushToken { return [[PushNotificationManager pushManager] getPushToken]; }
@end
