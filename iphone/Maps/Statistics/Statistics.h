#import <Foundation/Foundation.h>

@interface Statistics : NSObject
{
}

// Should be called from the same method in AppDelegate.
- (void)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions;
// Should be called from the same method in AppDelegate.
- (void)applicationDidBecomeActive;
- (void)logEvent:(NSString *)eventName;
- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void)logApiUsage:(NSString *)programName;
- (void)logLocation:(CLLocation *)location;

+ (instancetype)instance;

@property (nonatomic) BOOL enabled;

@end
