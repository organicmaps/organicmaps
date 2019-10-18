#import "StatisticsStrings.h"

typedef NS_ENUM(NSInteger, StatisticsChannel) {
  StatisticsChannelDefault,
  StatisticsChannelRealtime
};

@interface Statistics : NSObject

// Should be called from the same method in AppDelegate.
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions;
// Should be called from the same method in AppDelegate.
- (void)applicationDidBecomeActive;
- (void)logApiUsage:(NSString *)programName;

+ (instancetype)instance;
+ (void)logEvent:(NSString *)eventName;
+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location;
+ (void)logEvent:(NSString *)eventName withChannel:(StatisticsChannel)channel;
+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters withChannel:(StatisticsChannel)channel;
+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location
     withChannel:(StatisticsChannel)channel;

+ (NSString * _Nonnull)connectionTypeString;

@end
