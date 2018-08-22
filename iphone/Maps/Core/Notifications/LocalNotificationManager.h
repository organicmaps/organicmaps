#import "MWMTypes.h"

#import <UserNotifications/UserNotifications.h>

typedef void (^CompletionHandler)(UIBackgroundFetchResult);

@interface LocalNotificationManager : NSObject

+ (instancetype)sharedManager;

+ (BOOL)isLocalNotification:(UNNotification *)notification;

- (BOOL)showUGCNotificationIfNeeded:(MWMVoidBlock)onTap;
- (void)showDownloadMapNotificationIfNeeded:(CompletionHandler)completionHandler;
- (void)processNotification:(NSDictionary *)userInfo onLaunch:(BOOL)onLaunch;

@end
