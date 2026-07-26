#import "Logger.h"
#import <OSLog/OSLog.h>
#import "LogFileWriter.h"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "coding/zip_creator.hpp"

#include <atomic>
#include <string>
#include <vector>

@interface Logger ()

@property(nonnull, nonatomic) LogFileWriter * fileWriter;
@property(nonnull, nonatomic) os_log_t osLogger;
@property(class, readonly, nonatomic) dispatch_queue_t fileLoggingQueue;

+ (Logger *)logger;
+ (void)enableFileLogging;
+ (void)disableFileLogging;
+ (void)setFileLoggingState:(BOOL)enabled;
+ (void)handleFileLoggingError:(NSError *)error;
+ (void)runSyncOnFileLoggingQueue:(dispatch_block_t)block;
+ (void)logMessageWithLevel:(base::LogLevel)level src:(base::SrcPoint const &)src message:(std::string const &)message;
+ (void)tryWriteToFile:(std::string const &)logString;
+ (nullable NSString *)createTemporaryDirectory;
+ (nullable NSData *)archiveFiles:(NSArray<NSString *> *)filePaths inDirectory:(NSString *)directoryPath;
+ (nullable NSData *)archiveOSLogStoreReport;
+ (void)removeFileAtPath:(NSString *)filePath;
+ (void)reportSystemMessage:(NSString *)message type:(os_log_type_t)type;
+ (base::LogLevel)baseLevel:(LogLevel)level;

@end

// Subsystem and category are used for the OSLog.
NSString * const kLoggerSubsystem = [[NSBundle mainBundle] bundleIdentifier];
NSString * const kLoggerCategory = @"OM";
NSString * const kLogFileName = @"log.txt";
NSString * const kZipLogFileName = @"log.zip";

// Diagnostic logs are temporary support data. Caches keeps them out of backups (QA1719) and, unlike
// Documents, out of the Files app, which the app opens up with UIFileSharingEnabled.
NSString * const kLogDirectoryPath = [[NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:@"Logs"];
NSString * const kLegacyLogFilePath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:kLogFileName];

// Rotate a nonempty current file before a write would cross 16 MiB. One rotated file is retained.
// Oversized records stay intact and may exceed this target.
uint64_t constexpr kMaxLogFileSize = 16 * 1024 * 1024;

// The unified logging implementation silently cuts a single record and renders the loss as "<…>"
// on retrieval. The observed limit is 1015 bytes, but it is not a public constant, so stay below
// it with a margin and spend a part of the budget on saying what was lost.
size_t constexpr kMaxSystemLogRecordSize = 900;

/// @return |logString| shortened to kMaxSystemLogRecordSize bytes, marker included, so that the
/// truncation is explicit instead of being silently applied by the system.
static std::string TruncateForSystemLog(std::string const & logString)
{
  auto const marker = " [truncated, " + std::to_string(logString.size()) +
                      " bytes total; diagnostic file logging preserves full records when enabled]";
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
NSNotificationName const kFileLoggingStateDidChangeNotification = @"FileLoggingStateDidChangeNotification";

+ (void)initialize
{
  if (self == [Logger class])
  {
    SetLogMessageFn(&LogMessage);
    SetAssertFunction(&AssertMessage);
    // Documents is user-visible and backed up; remove any diagnostic log found there.
    dispatch_async([self fileLoggingQueue], ^{ [self removeFileAtPath:kLegacyLogFilePath]; });
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
  {
    _osLogger = os_log_create(kLoggerSubsystem.UTF8String, kLoggerCategory.UTF8String);
    _fileWriter = [[LogFileWriter alloc] initWithDirectoryPath:kLogDirectoryPath maxFileSize:kMaxLogFileSize];
  }
  return self;
}

// MARK: - Public

+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled
{
  BOOL const wasEnabled = self.fileLoggingEnabled;
  fileLoggingEnabled ? [self enableFileLogging] : [self disableFileLogging];
  BOOL const isEnabled = self.fileLoggingEnabled;
  if (!wasEnabled && isEnabled)
  {
    LOG_SHORT(LINFO, ("Local time:", NSDate.date.description.UTF8String,
                      ", Time Zone:", NSTimeZone.defaultTimeZone.abbreviation.UTF8String));
  }
  LOG(LINFO, ("File logging is enabled:", isEnabled ? "YES" : "NO"));
}

+ (BOOL)fileLoggingEnabled
{
  return _fileLoggingEnabled.load(std::memory_order_relaxed);
}

+ (NSNotificationName)fileLoggingStateDidChangeNotification
{
  return kFileLoggingStateDidChangeNotification;
}

+ (void)log:(LogLevel)level message:(NSString *)message
{
  LOG_SHORT([self baseLevel:level], (message.UTF8String));
}

+ (BOOL)canLog:(LogLevel)level
{
  return [Logger baseLevel:level] >= base::g_LogLevel;
}

+ (void)getLogArchiveWithCompletion:(void (^)(NSData * _Nullable archiveData))completion
{
  if (![self fileLoggingEnabled])
  {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ completion([self archiveOSLogStoreReport]); });
    return;
  }

  // Only the snapshot runs on the file logging queue: it drains the writes that are already
  // submitted and excludes the later ones. Archiving is seconds of work on a full log set, and
  // synchronous error records would be stuck behind it.
  dispatch_async([self fileLoggingQueue], ^{
    NSString * directoryPath = [self createTemporaryDirectory];
    if (directoryPath == nil)
    {
      completion(nil);
      return;
    }

    NSError * error = nil;
    NSArray<NSString *> * copies = [[self logger].fileWriter copyLogFilesToDirectory:directoryPath error:&error];
    if (copies == nil)
    {
      [self reportSystemMessage:[NSString stringWithFormat:@"Failed to snapshot the diagnostic log: %@",
                                                           error.localizedDescription]
                           type:OS_LOG_TYPE_ERROR];
      [self removeFileAtPath:directoryPath];
      completion(nil);
      return;
    }

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0),
                   ^{ completion([self archiveFiles:copies inDirectory:directoryPath]); });
  });
}

+ (void)flushWithCompletion:(dispatch_block_t)completion
{
  dispatch_async([self fileLoggingQueue], completion);
}

+ (void)flushSynchronously
{
  [self runSyncOnFileLoggingQueue:^{}];
}

+ (uint64_t)getLogFileSize
{
  return [self logger].fileWriter.totalFileSize;
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
    if ([self fileLoggingEnabled])
      return;

    NSError * error = nil;
    if (![[self logger].fileWriter openWithError:&error])
    {
      [self handleFileLoggingError:error];
      return;
    }
    [self setFileLoggingState:YES];
  }];
}

+ (void)disableFileLogging
{
  [self runSyncOnFileLoggingQueue:^{
    [self setFileLoggingState:NO];
    NSError * error = nil;
    if (![[self logger].fileWriter closeAndRemoveFilesWithError:&error])
      [self reportSystemMessage:error.localizedDescription type:OS_LOG_TYPE_ERROR];
  }];
}

+ (void)setFileLoggingState:(BOOL)enabled
{
  dispatch_assert_queue([self fileLoggingQueue]);
  bool const changed = _fileLoggingEnabled.exchange(enabled, std::memory_order_relaxed) != enabled;
  // Debug records are formatted only when a file can retain them. This mirrors Android's
  // LogsManager.nativeToggleCoreDebugLogs().
  base::g_LogLevel = enabled ? base::LDEBUG : base::GetDefaultLogLevel();
  if (!changed)
    return;

  dispatch_async(dispatch_get_main_queue(), ^{
    [NSNotificationCenter.defaultCenter postNotificationName:kFileLoggingStateDidChangeNotification object:nil];
  });
}

+ (void)handleFileLoggingError:(NSError *)error
{
  dispatch_assert_queue([self fileLoggingQueue]);
  [self setFileLoggingState:NO];

  NSError * cleanupError = nil;
  [[self logger].fileWriter closeAndRemoveFilesWithError:&cleanupError];
  NSString * message = [NSString stringWithFormat:@"File logging failed: %@", error.localizedDescription];
  if (cleanupError != nil)
    message = [message stringByAppendingFormat:@"; cleanup also failed: %@", cleanupError.localizedDescription];
  [self reportSystemMessage:message type:OS_LOG_TYPE_ERROR];
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
  if (![self fileLoggingEnabled])
    return;

  NSError * error = nil;
  NSData * data = [NSData dataWithBytes:logString.data() length:logString.size()];
  if (![[self logger].fileWriter writeData:data error:&error])
    [self handleFileLoggingError:error];
}

+ (void)runSyncOnFileLoggingQueue:(dispatch_block_t)block
{
  if (dispatch_get_specific(kFileLoggingQueueKey) != nullptr)
    block();
  else
    dispatch_sync([self fileLoggingQueue], block);
}

/// @return A new uniquely named temporary directory, so that repeated or concurrent reports cannot
/// overwrite or delete each other's files, or nil if it cannot be created.
+ (nullable NSString *)createTemporaryDirectory
{
  NSString * directoryPath = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  NSError * error = nil;
  if ([NSFileManager.defaultManager createDirectoryAtPath:directoryPath
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error])
    return directoryPath;

  [self reportSystemMessage:[NSString stringWithFormat:@"Failed to create a temporary directory: %@",
                                                       error.localizedDescription]
                       type:OS_LOG_TYPE_ERROR];
  return nil;
}

/// Archives |filePaths|, loads the result, and removes |directoryPath| on every return path.
+ (nullable NSData *)archiveFiles:(NSArray<NSString *> *)filePaths inDirectory:(NSString *)directoryPath
{
  NSData * archiveData = nil;
  if (filePaths.count == 0)
  {
    [self reportSystemMessage:@"There are no log files to export." type:OS_LOG_TYPE_DEFAULT];
  }
  else
  {
    std::vector<std::string> files;
    files.reserve(filePaths.count);
    for (NSString * filePath in filePaths)
      files.emplace_back(filePath.UTF8String);

    NSString * archivePath = [directoryPath stringByAppendingPathComponent:kZipLogFileName];
    if (!CreateZipFromFiles(files, archivePath.UTF8String))
    {
      [self reportSystemMessage:@"Failed to archive the log files." type:OS_LOG_TYPE_ERROR];
    }
    else
    {
      NSError * error = nil;
      archiveData = [NSData dataWithContentsOfFile:archivePath options:0 error:&error];
      if (archiveData == nil)
      {
        [self reportSystemMessage:[NSString stringWithFormat:@"Failed to read the log archive: %@",
                                                             error.localizedDescription]
                             type:OS_LOG_TYPE_ERROR];
      }
    }
  }

  [self removeFileAtPath:directoryPath];
  return archiveData;
}

/// Rebuilds a report from the system log for users who have not enabled file logging. The entries
/// are streamed into a file in chunks instead of being accumulated in memory first.
+ (nullable NSData *)archiveOSLogStoreReport
{
  NSError * error = nil;
  OSLogStore * store = [OSLogStore storeWithScope:OSLogStoreCurrentProcessIdentifier error:&error];
  if (store == nil)
  {
    [self reportSystemMessage:error.localizedDescription type:OS_LOG_TYPE_ERROR];
    return nil;
  }

  NSPredicate * predicate = [NSPredicate predicateWithFormat:@"subsystem == %@", kLoggerSubsystem];
  OSLogEnumerator * enumerator = [store entriesEnumeratorWithOptions:{} position:nil predicate:predicate error:&error];
  if (enumerator == nil)
  {
    [self reportSystemMessage:error.localizedDescription type:OS_LOG_TYPE_ERROR];
    return nil;
  }

  NSString * directoryPath = [self createTemporaryDirectory];
  if (directoryPath == nil)
    return nil;

  // Never the persistent log path: a later file logging session would append to a stale report.
  NSString * reportFilePath = [directoryPath stringByAppendingPathComponent:kLogFileName];
  if (![NSFileManager.defaultManager createFileAtPath:reportFilePath contents:nil attributes:nil])
  {
    [self reportSystemMessage:@"Failed to create the system log report." type:OS_LOG_TYPE_ERROR];
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  NSFileHandle * reportFile = [NSFileHandle fileHandleForWritingAtPath:reportFilePath];
  if (reportFile == nil)
  {
    [self reportSystemMessage:@"Failed to open the system log report." type:OS_LOG_TYPE_ERROR];
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  // NSFileHandle is unbuffered, and a report easily has tens of thousands of entries.
  NSUInteger constexpr kWriteBufferSize = 64 * 1024;
  NSMutableData * buffer = [NSMutableData dataWithCapacity:kWriteBufferSize];
  BOOL isEmpty = YES;
  BOOL writeSucceeded = YES;
  for (id entry in enumerator)
  {
    if (![entry isKindOfClass:OSLogEntryLog.class])
      continue;
    isEmpty = NO;
    NSString * line = [[entry composedMessage] stringByAppendingString:@"\n"];
    [buffer appendData:[line dataUsingEncoding:NSUTF8StringEncoding]];
    if (buffer.length >= kWriteBufferSize)
    {
      if (![reportFile writeData:buffer error:&error])
      {
        writeSucceeded = NO;
        break;
      }
      [buffer setLength:0];
    }
  }
  if (writeSucceeded && buffer.length > 0)
    writeSucceeded = [reportFile writeData:buffer error:&error];

  NSError * closeError = nil;
  if (![reportFile closeAndReturnError:&closeError] && writeSucceeded)
  {
    error = closeError;
    writeSucceeded = NO;
  }

  if (!writeSucceeded)
  {
    [self reportSystemMessage:[NSString stringWithFormat:@"Failed to write the system log report: %@",
                                                         error.localizedDescription]
                         type:OS_LOG_TYPE_ERROR];
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  if (isEmpty)
  {
    [self reportSystemMessage:@"The system log has no records to export." type:OS_LOG_TYPE_DEFAULT];
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  return [self archiveFiles:@[reportFilePath] inDirectory:directoryPath];
}

+ (void)removeFileAtPath:(NSString *)filePath
{
  NSError * error = nil;
  if ([NSFileManager.defaultManager removeItemAtPath:filePath error:&error])
    return;
  if ([error.domain isEqualToString:NSCocoaErrorDomain] && error.code == NSFileNoSuchFileError)
    return;
  [self reportSystemMessage:error.localizedDescription type:OS_LOG_TYPE_ERROR];
}

+ (void)reportSystemMessage:(NSString *)message type:(os_log_type_t)type
{
  os_log_with_type([self logger].osLogger, type, "%{public}s", message.UTF8String);
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
