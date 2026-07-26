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
@property(class, readonly, nonatomic) NSNotificationName fileLoggingStateDidChangeNotification;

+ (void)log:(LogLevel)level message:(NSString *)message;
+ (BOOL)canLog:(LogLevel)level;

/// Creates a zipped diagnostic log, or rebuilds one from the system log when file logging is off.
/// The completion runs on a background queue with nil if the archive cannot be created.
+ (void)getLogArchiveWithCompletion:(void (^)(NSData * _Nullable archiveData))completion;

/// Calls |completion| on the file logging queue after all previously submitted writes finish.
+ (void)flushWithCompletion:(dispatch_block_t)completion;

/// Waits for all previously submitted writes. Lifecycle code should prefer the asynchronous flush
/// whenever the process is expected to continue running.
+ (void)flushSynchronously;

/// Total size of the diagnostic log files.
+ (uint64_t)getLogFileSize;

/// Removes the log that versions before the move to Application Support left in Documents, which is
/// user-visible and backed up. Documents is writable by the user, so run this migration only once.
+ (void)removeLegacyLogFile;

@end

NS_ASSUME_NONNULL_END
