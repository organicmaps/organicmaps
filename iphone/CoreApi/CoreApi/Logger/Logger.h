#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LogLevel) {
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelCritical
};

@interface Logger : NSObject

/// Detailed diagnostic logging into a file. Enabling it also unlocks the debug log level.
/// Reading it back reports the state the logger is actually in: enabling fails if the log
/// file cannot be opened.
@property(class, nonatomic) BOOL fileLoggingEnabled;

+ (void)log:(LogLevel)level message:(NSString *)message;
+ (BOOL)canLog:(LogLevel)level;
+ (nullable NSURL *)getLogFileURL;
+ (uint64_t)getLogFileSize;

@end

NS_ASSUME_NONNULL_END
