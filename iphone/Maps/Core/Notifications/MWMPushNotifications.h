#import <UserNotifications/UserNotifications.h>

@interface MWMPushNotifications : NSObject

+ (void)setup;

+ (void)application:(UIApplication *_Nonnull)application
didRegisterForRemoteNotificationsWithDeviceToken:(NSData *_Nonnull)deviceToken;
+ (void)application:(UIApplication *_Nonnull)application
didFailToRegisterForRemoteNotificationsWithError:(NSError *_Nonnull)error;
+ (void)application:(UIApplication *_Nonnull)application
didReceiveRemoteNotification:(NSDictionary *_Nonnull)userInfo
fetchCompletionHandler:(void (^_Nullable)(UIBackgroundFetchResult))completionHandler;
+ (void)userNotificationCenter:(UNUserNotificationCenter *_Nonnull)center
       willPresentNotification:(UNNotification *_Nonnull)notification
         withCompletionHandler:(void (^_Nullable)(UNNotificationPresentationOptions options))completionHandler;
+ (void)userNotificationCenter:(UNUserNotificationCenter *_Nonnull)center
didReceiveNotificationResponse:(UNNotificationResponse *_Nonnull)response
         withCompletionHandler:(void(^_Nullable)(void))completionHandler;

+ (NSString * _Nonnull)formattedTimestamp;

@end
