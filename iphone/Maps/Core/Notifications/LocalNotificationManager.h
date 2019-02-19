NS_ASSUME_NONNULL_BEGIN

@class CoreNotificationWrapper;

@interface LocalNotificationManager : NSObject

+ (BOOL)shouldShowAuthNotification;
+ (void)authNotificationWasShown;
+ (CoreNotificationWrapper * _Nullable)reviewNotificationWrapper;
+ (void)reviewNotificationWasShown;

@end

NS_ASSUME_NONNULL_END
