#import <Foundation/Foundation.h>

#include "../../stats/client/stats_client.hpp"

@interface Statistics : NSObject
{
  stats::Client m_client;
}

- (void)startSession;
- (void)logEvent:(NSString *)eventName;
- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void)logProposalReason:(NSString *)reason withAnswer:(NSString *)answer;
- (void)logApiUsage:(NSString *)programName;
- (void)logLatitude:(double)latitude longitude:(double)longitude horizontalAccuracy:(double)horizontalAccuracy verticalAccuracy:(double)verticalAccuracy;
- (void)logSearchQuery:(NSString *)query;

+ (id)instance;

@property (nonatomic, readonly) BOOL enabled;

@end
