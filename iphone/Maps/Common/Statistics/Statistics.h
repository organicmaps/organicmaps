#import "StatisticsStrings.h"

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

@end
