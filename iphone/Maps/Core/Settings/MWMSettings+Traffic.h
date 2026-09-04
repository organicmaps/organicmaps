#import "MWMSettings.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(TrafficSettings)
@protocol MWMTrafficSettings

+ (NSString *)trafficApiKey;
+ (void)setTrafficApiKey:(NSString *)apiKey;
+ (BOOL)isWellFormedTrafficApiKey:(NSString *)apiKey;

@end

@interface MWMSettings (Traffic) <MWMTrafficSettings>
@end

NS_ASSUME_NONNULL_END
