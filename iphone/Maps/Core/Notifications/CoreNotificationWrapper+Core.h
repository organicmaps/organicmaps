#import "CoreNotificationWrapper.h"

#include "map/notifications/notification_manager.hpp"

@interface CoreNotificationWrapper (Core)

- (instancetype)initWithNotificationCandidate:(notifications::NotificationCandidate const &)notification;
- (notifications::NotificationCandidate)notificationCandidate;

@end
