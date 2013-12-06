#import <Foundation/Foundation.h>

@interface Statistics : NSObject

- (void)startSession;
- (void)logEvent:(NSString *)eventName;
- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void)logProposalReason:(NSString *)reason withAnswer:(NSString *)answer;
- (void)logApiUsage:(NSString *)programName;
- (void)logLatitude:(double)latitude longitude:(double)longitude horizontalAccuracy:(double)horizontalAccuracy verticalAccuracy:(double)verticalAccuracy;

+ (id)instance;

@property (nonatomic, readonly) BOOL enabled;

@end
