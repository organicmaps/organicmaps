
#import <Foundation/Foundation.h>

@interface LocalNotificationManager : NSObject

+ (instancetype)sharedManager;

- (void)showDownloadMapNotificationIfNeeded:(void (^)(UIBackgroundFetchResult))completionHandler;
- (void)showDownloadMapAlertIfNeeded;
- (void)processNotification:(UILocalNotification *)notification;

- (void)schedulePromoNotification;

@end
