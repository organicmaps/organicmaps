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

/// Creates a zipped copy of the diagnostic log, or of a report rebuilt from the system log when
/// file logging is off. Everything is done on a background queue, and the completion is called
/// there too, with nil if the archive cannot be created. The caller owns the returned file and is
/// expected to remove its enclosing temporary directory.
+ (void)getLogFileURLWithCompletion:(void (^)(NSURL * _Nullable logFileURL))completion;

/// Total size of the diagnostic log files.
+ (uint64_t)getLogFileSize;

@end

NS_ASSUME_NONNULL_END
