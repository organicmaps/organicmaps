#import "Logger.h"
#import <OSLog/OSLog.h>
#import <UIKit/UIKit.h>

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "coding/zip_creator.hpp"

#include <atomic>
#include <string>

@interface Logger ()

@property(nullable, nonatomic) NSFileHandle * fileHandle;
@property(nonnull, nonatomic) os_log_t osLogger;
@property(class, readonly, nonatomic) dispatch_queue_t fileLoggingQueue;

+ (Logger *)logger;
+ (void)enableFileLogging;
+ (void)disableFileLogging;
+ (void)runSyncOnFileLoggingQueue:(dispatch_block_t)block;
+ (void)logMessageWithLevel:(base::LogLevel)level src:(base::SrcPoint const &)src message:(std::string const &)message;
+ (void)tryWriteToFile:(std::string const &)logString;
+ (NSURL *)getZippedLogFile:(NSString *)logFilePath;
+ (void)removeFileAtPath:(NSString *)filePath;
+ (base::LogLevel)baseLevel:(LogLevel)level;

@end

// Subsystem and category are used for the OSLog.
NSString * const kLoggerSubsystem = [[NSBundle mainBundle] bundleIdentifier];
NSString * const kLoggerCategory = @"OM";
NSString * const kLogFileName = @"log.txt";
NSString * const kZipLogFileExtension = @"zip";
NSString * const kLogFilePath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:kLogFileName];
// TODO: (KK) Review and change this limit after some testing.
NSUInteger const kMaxLogFileSize = 1024 * 1024 * 100;  // 100 MB;

// The unified logging implementation silently cuts a single record and renders the loss as "<…>"
// on retrieval. The observed limit is 1015 bytes, but it is not a public constant, so stay below
// it with a margin and spend a part of the budget on saying what was lost.
size_t constexpr kMaxSystemLogRecordSize = 900;

/// @return |logString| shortened to kMaxSystemLogRecordSize bytes, marker included, so that the
/// truncation is explicit instead of being silently applied by the system.
static std::string TruncateForSystemLog(std::string const & logString)
{
  auto const marker = " [truncated, " + std::to_string(logString.size()) +
                      " bytes total, enable logging in Settings to get the full record]";
  ASSERT_LESS(marker.size(), kMaxSystemLogRecordSize, ());
  auto const prefix = strings::TruncateUtf8(logString, kMaxSystemLogRecordSize - marker.size());
  return std::string{prefix} + marker;
}

static os_log_type_t OSLogTypeFor(base::LogLevel level)
{
  switch (level)
  {
  case base::LDEBUG: return OS_LOG_TYPE_DEBUG;
  // Deliberately not OS_LOG_TYPE_INFO: unlike the default one it is not persisted, and bug
  // reports are reconstructed from the OSLog store when file logging is off.
  case base::LINFO:
  case base::LWARNING: return OS_LOG_TYPE_DEFAULT;
  // OS_LOG_TYPE_FAULT is reserved for system-level failures, LCRITICAL is a process-level one.
  case base::LERROR:
  case base::LCRITICAL: return OS_LOG_TYPE_ERROR;
  case base::NUM_LOG_LEVELS: return OS_LOG_TYPE_DEFAULT;
  }
}

@implementation Logger

/// Read from any thread on every log call, mutated on the fileLoggingQueue only.
static std::atomic<bool> _fileLoggingEnabled{false};
static void * kFileLoggingQueueKey = &kFileLoggingQueueKey;

+ (void)initialize
{
  if (self == [Logger class])
  {
    SetLogMessageFn(&LogMessage);
    SetAssertFunction(&AssertMessage);
    // Ordinary records are written asynchronously, so the ones that are still queued would be lost
    // if the app is killed while suspended. An empty block on the serial queue waits for them.
    [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidEnterBackgroundNotification
                                                    object:nil
                                                     queue:nil
                                                usingBlock:^(NSNotification *) {
                                                  if ([self fileLoggingEnabled])
                                                    [self runSyncOnFileLoggingQueue:^{}];
                                                }];
  }
}

+ (Logger *)logger
{
  static Logger * logger = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ logger = [[self alloc] init]; });
  return logger;
}

+ (dispatch_queue_t)fileLoggingQueue
{
  static dispatch_queue_t fileLoggingQueue = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    dispatch_queue_attr_t attributes =
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_UTILITY, 0);
    fileLoggingQueue = dispatch_queue_create("app.organicmaps.fileLoggingQueue", attributes);
    dispatch_queue_set_specific(fileLoggingQueue, kFileLoggingQueueKey, kFileLoggingQueueKey, nullptr);
  });
  return fileLoggingQueue;
}

- (instancetype)init
{
  self = [super init];
  if (self)
    _osLogger = os_log_create(kLoggerSubsystem.UTF8String, kLoggerCategory.UTF8String);
  return self;
}

// MARK: - Public

+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled
{
  fileLoggingEnabled ? [self enableFileLogging] : [self disableFileLogging];
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    LOG_SHORT(LINFO, ("Local time:", NSDate.date.description.UTF8String,
                      ", Time Zone:", NSTimeZone.defaultTimeZone.abbreviation.UTF8String));
  });
  LOG(LINFO, ("File logging is enabled:", [self fileLoggingEnabled] ? "YES" : "NO"));
}

+ (BOOL)fileLoggingEnabled
{
  return _fileLoggingEnabled.load(std::memory_order_relaxed);
}

+ (void)log:(LogLevel)level message:(NSString *)message
{
  LOG_SHORT([self baseLevel:level], (message.UTF8String));
}

+ (BOOL)canLog:(LogLevel)level
{
  return [Logger baseLevel:level] >= base::g_LogLevel;
}

+ (nullable NSURL *)getLogFileURL
{
  if ([self fileLoggingEnabled])
  {
    // Drain the writes that are already submitted to the serial queue.
    [self runSyncOnFileLoggingQueue:^{}];
    if (![NSFileManager.defaultManager fileExistsAtPath:kLogFilePath])
    {
      LOG(LERROR, ("Log file doesn't exist while file logging is enabled:", kLogFilePath.UTF8String));
      return nil;
    }
    return [self getZippedLogFile:kLogFilePath];
  }
  else
  {
    // Fetch logs from the OSLog store.
    if (@available(iOS 15.0, *))
    {
      NSError * error;
      OSLogStore * store = [OSLogStore storeWithScope:OSLogStoreCurrentProcessIdentifier error:&error];

      if (error)
      {
        LOG(LERROR, (error.localizedDescription.UTF8String));
        return nil;
      }

      NSPredicate * predicate = [NSPredicate predicateWithFormat:@"subsystem == %@", kLoggerSubsystem];
      OSLogEnumerator * enumerator = [store entriesEnumeratorWithOptions:{}
                                                                position:nil
                                                               predicate:predicate
                                                                   error:&error];

      if (error)
      {
        LOG(LERROR, (error.localizedDescription.UTF8String));
        return nil;
      }

      NSMutableString * logString = [NSMutableString string];
      NSString * kNewLineStr = @"\n";

      id object;
      while (object = [enumerator nextObject])
      {
        if ([object isMemberOfClass:[OSLogEntryLog class]])
        {
          [logString appendString:[object composedMessage]];
          [logString appendString:kNewLineStr];
        }
      }

      if (logString.length == 0)
      {
        LOG(LINFO, ("OSLog entry is empty."));
        return nil;
      }

      [NSFileManager.defaultManager createFileAtPath:kLogFilePath
                                            contents:[logString dataUsingEncoding:NSUTF8StringEncoding]
                                          attributes:nil];
      return [self getZippedLogFile:kLogFilePath];
    }
    else
    {
      return nil;
    }
  }
}

+ (uint64_t)getLogFileSize
{
  __block uint64_t fileSize = 0;
  [self runSyncOnFileLoggingQueue:^{
    NSFileHandle * fileHandle = [self logger].fileHandle;
    fileSize = fileHandle != nil ? [fileHandle offsetInFile] : 0;
  }];
  return fileSize;
}

// MARK: - C++ injection

void LogMessage(base::LogLevel level, base::SrcPoint const & src, std::string const & message)
{
  [Logger logMessageWithLevel:level src:src message:message];
  CHECK_LESS(level, base::g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

bool AssertMessage(base::SrcPoint const & src, std::string const & message)
{
  [Logger logMessageWithLevel:base::LCRITICAL src:src message:message];
  return true;
}

// MARK: - Private

+ (void)enableFileLogging
{
  [self runSyncOnFileLoggingQueue:^{
    Logger * logger = [self logger];
    NSFileManager * fileManager = [NSFileManager defaultManager];

    // Create a log file if it doesn't exist and setup file handle for writing.
    if (![fileManager fileExistsAtPath:kLogFilePath])
      [fileManager createFileAtPath:kLogFilePath contents:nil attributes:nil];
    NSFileHandle * fileHandle = [NSFileHandle fileHandleForWritingAtPath:kLogFilePath];
    if (fileHandle == nil)
    {
      LOG(LERROR, ("Failed to open log file for writing", kLogFilePath.UTF8String));
      [self disableFileLogging];
      return;
    }
    // Clean up the file if it exceeds the maximum size.
    if ([fileManager contentsAtPath:kLogFilePath].length > kMaxLogFileSize)
      [fileHandle truncateFileAtOffset:0];

    logger.fileHandle = fileHandle;

    _fileLoggingEnabled.store(true, std::memory_order_relaxed);
    // Debug records are worth formatting only when there is a file to keep them in, so the level
    // follows the state the logger is actually in. Mirrors Android's
    // LogsManager.nativeToggleCoreDebugLogs().
    base::g_LogLevel = base::LDEBUG;
  }];
}

+ (void)disableFileLogging
{
  [self runSyncOnFileLoggingQueue:^{
    Logger * logger = [self logger];

    _fileLoggingEnabled.store(false, std::memory_order_relaxed);
    base::g_LogLevel = base::GetDefaultLogLevel();

    [logger.fileHandle closeFile];
    logger.fileHandle = nil;
    [self removeFileAtPath:kLogFilePath];
  }];
}

+ (void)logMessageWithLevel:(base::LogLevel)level src:(base::SrcPoint const &)src message:(std::string const &)message
{
  // Build the log message string.
  std::ostringstream output;
  base::LogHelper::WriteProlog(output, level);
  base::LogHelper::WriteLog(output, src, message);

  auto const logString = output.str();

  // Log the message into the system log. Oversized records are rare, so nothing is copied here
  // unless one has to be rebuilt with a truncation marker.
  std::string truncated;
  if (logString.size() > kMaxSystemLogRecordSize)
    truncated = TruncateForSystemLog(logString);
  os_log_with_type([self logger].osLogger, OSLogTypeFor(level), "%{public}s",
                   truncated.empty() ? logString.c_str() : truncated.c_str());

  if (!_fileLoggingEnabled.load(std::memory_order_relaxed))
    return;

  // Write errors synchronously: an error is often the last record before the process dies - through
  // the abort in LogMessage, or in the code that reported it - and that is exactly the record a
  // diagnostic log is collected for. Everything else goes asynchronously to keep the calling
  // (usually the main) thread out of the file I/O. The queue is serial, so the order is preserved.
  if (level >= base::LERROR)
    [self runSyncOnFileLoggingQueue:^{ [self tryWriteToFile:logString]; }];
  else
    dispatch_async([self fileLoggingQueue], ^{ [self tryWriteToFile:logString]; });
}

+ (void)tryWriteToFile:(std::string const &)logString
{
  dispatch_assert_queue([self fileLoggingQueue]);
  NSFileHandle * fileHandle = [self logger].fileHandle;
  if (fileHandle != nil)
  {
    [fileHandle seekToEndOfFile];
    [fileHandle writeData:[NSData dataWithBytes:logString.c_str() length:logString.length()]];
  }
}

+ (void)runSyncOnFileLoggingQueue:(dispatch_block_t)block
{
  if (dispatch_get_specific(kFileLoggingQueueKey) != nullptr)
    block();
  else
    dispatch_sync([self fileLoggingQueue], block);
}

+ (NSURL *)getZippedLogFile:(NSString *)logFilePath
{
  NSString * zipFileName = [[logFilePath.lastPathComponent stringByDeletingPathExtension]
      stringByAppendingPathExtension:kZipLogFileExtension];
  NSString * zipFilePath =
      [[NSFileManager.defaultManager temporaryDirectory] URLByAppendingPathComponent:zipFileName].path;
  auto const success = CreateZipFromFiles({logFilePath.UTF8String}, zipFilePath.UTF8String);
  if (!success)
  {
    LOG(LERROR, ("Failed to zip log file:", kLogFilePath.UTF8String, ". The original file will be returned."));
    return [NSURL fileURLWithPath:logFilePath];
  }
  return [NSURL fileURLWithPath:zipFilePath];
}

+ (void)removeFileAtPath:(NSString *)filePath
{
  if ([NSFileManager.defaultManager fileExistsAtPath:filePath])
  {
    NSError * error;
    [NSFileManager.defaultManager removeItemAtPath:filePath error:&error];
    if (error)
      LOG(LERROR, (error.localizedDescription.UTF8String));
  }
}

+ (base::LogLevel)baseLevel:(LogLevel)level
{
  switch (level)
  {
  case LogLevelDebug: return LDEBUG;
  case LogLevelInfo: return LINFO;
  case LogLevelWarning: return LWARNING;
  case LogLevelError: return LERROR;
  case LogLevelCritical: return LCRITICAL;
  }
}

@end
