#import <Foundation/Foundation.h>

@interface Statistics : NSObject

- (void) startSession;
- (void) stopSession;
- (void) logEvent:(NSString *)eventName;
- (void) logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters;
- (void) logProposalReason:(NSString *)reason withAnswer:(NSString *)answer;
- (void)logApiUsage:(NSString *)programName;

+ (id) instance;

@end
