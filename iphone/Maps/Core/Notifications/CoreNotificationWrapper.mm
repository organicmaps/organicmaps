#import "CoreNotificationWrapper.h"
#import "CoreNotificationWrapper+Core.h"

static NSString *const kX = @"x";
static NSString *const kY = @"y";
static NSString *const kType = @"type";
static NSString *const kName = @"name";
static NSString *const kReadableName = @"readableName";

@implementation CoreNotificationWrapper

- (instancetype)initWithNotificationDictionary:(NSDictionary *)dictionary {
  self = [super init];
  if (self) {
    NSNumber *x = dictionary[kX];
    NSNumber *y = dictionary[kY];
    NSString *type = dictionary[kType];
    NSString *name = dictionary[kName];
    NSString *readableName = dictionary[kReadableName];
    if (x && y && type && name && readableName) {
      _x = x.doubleValue;
      _y = y.doubleValue;
      _bestType = type;
      _defaultName = name;
      _readableName = readableName;
    } else {
      return nil;
    }
  }

  return self;
}

- (NSDictionary *)notificationDictionary {
  return @{
    kX : @(self.x),
    kY : @(self.y),
    kType : self.bestType,
    kName : self.defaultName,
    kReadableName : self.readableName
  };
}

@end

@implementation CoreNotificationWrapper (Core)

- (instancetype)initWithNotificationCandidate:(notifications::NotificationCandidate const &)notification {
  self = [super init];
  if (self) {
    _x = notification.GetPos().x;
    _y = notification.GetPos().y;
    _defaultName = @(notification.GetDefaultName().c_str());
    _bestType = @(notification.GetBestFeatureType().c_str());
    _readableName = @(notification.GetReadableName().c_str());
    _address = @(notification.GetAddress().c_str());
  }

  return self;
}

- (notifications::NotificationCandidate)notificationCandidate {
  notifications::NotificationCandidate nc(notifications::NotificationCandidate::Type::UgcReview);
  nc.SetDefaultName(self.defaultName.UTF8String);
  nc.SetBestFeatureType(self.bestType.UTF8String);
  nc.SetPos(m2::PointD(self.x, self.y));
  nc.SetReadableName(self.readableName.UTF8String);
  return nc;
}

@end
