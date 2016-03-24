#import "StatisticsStrings.h"

@interface Statistics : NSObject
+ (bool)isStatisticsEnabledByDefault;
- (bool)isStatisticsEnabled;
- (void)enableOnNextAppLaunch;
- (void)disableOnNextAppLaunch;

// Should be called from the same method in AppDelegate.
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions;
// Should be called from the same method in AppDelegate.
- (void)applicationDidBecomeActive;
- (void)logApiUsage:(NSString *)programName;
- (void)logLocation:(CLLocation *)location;

+ (instancetype)instance;
+ (void)logEvent:(NSString *)eventName;
+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;

@end
