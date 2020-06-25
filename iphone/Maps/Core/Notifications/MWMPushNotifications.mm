#import "MWMPushNotifications.h"
#import <FirebaseCrashlytics/FirebaseCrashlytics.h>
#import <Pushwoosh/PushNotificationManager.h>
#import "Statistics.h"

#include "platform/marketing_service.hpp"
#include "platform/platform.hpp"

#import "3party/Alohalytics/src/alohalytics_objc.h"

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.

namespace
{
NSString * const kPushDeviceTokenLogEvent = @"iOSPushDeviceToken";
}  // namespace

@implementation MWMPushNotifications

+ (void)setup
{
  PushNotificationManager * pushManager = [PushNotificationManager pushManager];

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
  NSLog(@"Pushwoosh token: %@", [pushManager getPushToken]);
  [Alohalytics logEvent:kPushDeviceTokenLogEvent withValue:pushManager.getHWID];
}

+ (void)application:(UIApplication *)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
  [[PushNotificationManager pushManager] handlePushRegistrationFailure:error];
  [[FIRCrashlytics crashlytics] recordError:error];
}

+ (void)application:(UIApplication *)application
    didReceiveRemoteNotification:(NSDictionary *)userInfo
          fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  [Statistics logEvent:kStatEventName(kStatApplication, kStatPushReceived) withParameters:userInfo];
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
  [[PushNotificationManager pushManager].notificationCenterDelegate userNotificationCenter:center
                                                                   willPresentNotification:notification
                                                                     withCompletionHandler:completionHandler];
}

+ (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void(^)(void))completionHandler
{
  [[PushNotificationManager pushManager].notificationCenterDelegate userNotificationCenter:center
                                                            didReceiveNotificationResponse:response
                                                                     withCompletionHandler:completionHandler];
}

+ (NSString * _Nonnull)formattedTimestamp {
  return @(GetPlatform().GetMarketingService().GetPushWooshTimestamp().c_str());
}

@end
