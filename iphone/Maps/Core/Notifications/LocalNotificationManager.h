#import "MWMTypes.h"

typedef void (^CompletionHandler)(UIBackgroundFetchResult);

@interface LocalNotificationManager : NSObject

+ (instancetype)sharedManager;

- (BOOL)showUGCNotificationIfNeeded:(MWMVoidBlock)onTap;

- (void)showDownloadMapNotificationIfNeeded:(CompletionHandler)completionHandler;
- (void)processNotification:(NSDictionary *)userInfo onLaunch:(BOOL)onLaunch;

@end
