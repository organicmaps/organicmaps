
#import <Foundation/Foundation.h>

@interface LocalNotificationManager : NSObject

+ (instancetype)sharedManager;

- (void)showDownloadMapNotificationIfNeeded:(void (^)(UIBackgroundFetchResult))completionHandler;
- (void)showDownloadMapAlertIfNeeded;
- (void)processDownloadMapNotification:(UILocalNotification *)notification;

@end
