#import <Foundation/Foundation.h>

@interface Statistics : NSObject
{
}

- (void)startSessionWithLaunchOptions:(NSDictionary *)launchOptions;
- (void)logEvent:(NSString *)eventName;
- (void)logInAppMessageEvent:(NSString *)eventName imageType:(NSString *)imageType;
- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void)logApiUsage:(NSString *)programName;
- (void)logLatitude:(double)latitude longitude:(double)longitude horizontalAccuracy:(double)horizontalAccuracy verticalAccuracy:(double)verticalAccuracy;

+ (id)instance;

@property (nonatomic, readonly) BOOL enabled;

@end
