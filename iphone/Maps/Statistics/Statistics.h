#import <Foundation/Foundation.h>

#include "../../stats/client/event_tracker.hpp"

@interface Statistics : NSObject
{
  stats::EventTracker m_tracker;
}

- (void)startSessionWithLaunchOptions:(NSDictionary *)launchOptions;
- (void)logEvent:(NSString *)eventName;
- (void)logInAppMessageEvent:(NSString *)eventName imageType:(NSString *)imageType;
- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void)logProposalReason:(NSString *)reason withAnswer:(NSString *)answer;
- (void)logApiUsage:(NSString *)programName;
- (void)logLatitude:(double)latitude longitude:(double)longitude horizontalAccuracy:(double)horizontalAccuracy verticalAccuracy:(double)verticalAccuracy;
- (void)logSearchQuery:(NSString *)query;

- (void)applicationWillTerminate;
- (void)applicationDidEnterBackground;
- (void)applicationWillResignActive;
- (void)applicationWillEnterForeground;
- (void)applicationDidBecomeActive;

+ (id)instance;

@property (nonatomic, readonly) BOOL enabled;

@end
