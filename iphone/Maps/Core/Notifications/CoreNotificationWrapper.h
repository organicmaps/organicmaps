#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CoreNotificationWrapper : NSObject

@property(nonatomic, copy) NSString *bestType;
@property(nonatomic, copy) NSString *defaultName;
@property(nonatomic, copy) NSString *readableName;
@property(nonatomic, copy) NSString *address;
@property(nonatomic) double x;
@property(nonatomic) double y;

- (instancetype _Nullable)initWithNotificationDictionary:(NSDictionary *)dictionary;
- (NSDictionary *)notificationDictionary;

@end

NS_ASSUME_NONNULL_END
