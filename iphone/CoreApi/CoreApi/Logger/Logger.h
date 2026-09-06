#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LogLevel) {
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelCritical
};

typedef void (^LogArchiveCompletion)(NSData * _Nullable archiveData, NSError * _Nullable error);

@interface Logger : NSObject

/// Detailed diagnostic logging into a file. Enabling it also unlocks the debug log level.
/// Reading it back reports the state the logger is actually in: enabling fails if the log
/// file cannot be opened.
@property(class, nonatomic) BOOL fileLoggingEnabled;
/// The notification object is the error that stopped file logging, or nil for a requested state
/// change.
@property(class, readonly, nonatomic) NSNotificationName fileLoggingStateDidChangeNotification;

/// Applies the requested state synchronously and reports an open, close or removal failure.
+ (BOOL)setFileLoggingEnabled:(BOOL)enabled error:(NSError * __autoreleasing _Nullable * _Nullable)error;

+ (void)log:(LogLevel)level message:(NSString *)message;
+ (BOOL)canLog:(LogLevel)level;

/// Creates a zipped diagnostic report from the system log and every retained file log. Exactly one
/// completion argument is nonnull, and the completion always runs on a background utility queue.
+ (void)getLogArchiveWithCompletion:(LogArchiveCompletion)completion;

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
