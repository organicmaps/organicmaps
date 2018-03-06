//
//  PWLog.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2016
//

#import <Foundation/Foundation.h>

typedef NSString *PWLogLevel NS_EXTENSIBLE_STRING_ENUM;

extern PWLogLevel const PWLogLevelNone;
extern PWLogLevel const PWLogLevelError;
extern PWLogLevel const PWLogLevelWarning;
extern PWLogLevel const PWLogLevelInfo;
extern PWLogLevel const PWLogLevelDebug;
extern PWLogLevel const PWLogLevelVerbose;

@interface PWLog : NSObject

+ (void)setLogsHandler:(void(^)(PWLogLevel level, NSString *description))logsHandler;
+ (void)removeLogsHandler;

@end
