#import "Logger.h"
#import <OSLog/OSLog.h>
#import "LogFileWriter.h"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/zip_creator.hpp"

#include <atomic>
#include <string>
#include <vector>

@interface Logger ()

@property(nonnull, nonatomic) LogFileWriter * fileWriter;
@property(nonnull, nonatomic) os_log_t osLogger;

@end

namespace
{
// Subsystem and category are used for the OSLog.
NSString * const kLoggerSubsystem = [[NSBundle mainBundle] bundleIdentifier];
NSString * const kLoggerCategory = @"OM";
NSString * const kLogFileName = @"log.txt";
NSString * const kSystemLogFileName = @"system-log.txt";
NSString * const kZipLogFileName = @"log.zip";
NSString * const kLoggerErrorDomain = @"app.organicmaps.Logger";

// A diagnostic log is collected on request and has to survive until the user sends it, so it cannot
// live in Caches, which the system purges under storage pressure. Application Support is not
// exposed to the Files app the way Documents is, and LogFileWriter excludes it from backups.
NSString * const kLogDirectoryPath =
    [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject]
        stringByAppendingPathComponent:@"Logs"];
NSString * const kLegacyLogFilePath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:kLogFileName];

// Rotate a nonempty current file before a write would cross 16 MiB. One rotated file is retained.
// Oversized records stay intact and may exceed this target.
uint64_t constexpr kMaxLogFileSize = 16 * 1024 * 1024;

// The unified logging implementation silently cuts a single record and renders the loss as "<…>"
// on retrieval. The observed limit is 1015 bytes, but it is not a public constant, so stay below
// it with a margin and spend a part of the budget on saying what was lost.
size_t constexpr kMaxSystemLogRecordSize = 900;

typedef NS_ERROR_ENUM(kLoggerErrorDomain, LoggerError){
    LoggerErrorCreateTemporaryDirectory = 1,
    LoggerErrorSystemLogUnavailable,
    LoggerErrorNoLogs,
    LoggerErrorArchiveFailed,
    LoggerErrorReadArchiveFailed,
};

NSError * MakeLoggerError(LoggerError code, NSString * description)
{
  return [NSError errorWithDomain:kLoggerErrorDomain code:code userInfo:@{NSLocalizedDescriptionKey: description}];
}

/// @return |logString| shortened to kMaxSystemLogRecordSize bytes, marker included, so that the
/// truncation is explicit instead of being silently applied by the system.
std::string TruncateForSystemLog(std::string const & logString)
{
  auto const marker = " [truncated, " + std::to_string(logString.size()) +
                      " bytes total; diagnostic file logging preserves full records when enabled]";
  ASSERT_LESS(marker.size(), kMaxSystemLogRecordSize, ());
  auto const prefix = strings::TruncateUtf8(logString, kMaxSystemLogRecordSize - marker.size());
  return std::string{prefix} + marker;
}

os_log_type_t OSLogTypeFor(base::LogLevel level)
{
  switch (level)
  {
  case base::LDEBUG: return OS_LOG_TYPE_DEBUG;
  // Deliberately not OS_LOG_TYPE_INFO: unlike the default one it is not persisted, and every bug
  // report includes the OSLog store.
  case base::LINFO:
  case base::LWARNING: return OS_LOG_TYPE_DEFAULT;
  // OS_LOG_TYPE_FAULT is reserved for system-level failures, LCRITICAL is a process-level one.
  case base::LERROR:
  case base::LCRITICAL: return OS_LOG_TYPE_ERROR;
  case base::NUM_LOG_LEVELS: return OS_LOG_TYPE_DEFAULT;
  }
}

/// Read from any thread on every log call, mutated on the fileLoggingQueue only.
std::atomic<bool> g_fileLoggingEnabled{false};
void * kFileLoggingQueueKey = &kFileLoggingQueueKey;
NSNotificationName const kFileLoggingStateDidChangeNotification = @"FileLoggingStateDidChangeNotification";
}  // namespace

@implementation Logger

+ (void)initialize
{
  if (self == [Logger class])
  {
    SetLogMessageFn(&LogMessage);
    SetAssertFunction(&AssertMessage);
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
  [self setFileLoggingEnabled:fileLoggingEnabled error:nil];
}

+ (BOOL)setFileLoggingEnabled:(BOOL)fileLoggingEnabled error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  BOOL const wasEnabled = self.fileLoggingEnabled;
  NSError * operationError = nil;
  BOOL const succeeded = fileLoggingEnabled ? [self enableFileLoggingWithError:&operationError]
                                            : [self disableFileLoggingWithError:&operationError];
  BOOL const isEnabled = self.fileLoggingEnabled;
  if (!wasEnabled && isEnabled)
  {
    LOG_SHORT(LINFO, ("Local time:", NSDate.date.description.UTF8String,
                      ", Time Zone:", NSTimeZone.defaultTimeZone.abbreviation.UTF8String));
  }
  LOG(LINFO, ("File logging is enabled:", isEnabled ? "YES" : "NO"));
  if (error != nullptr)
    *error = operationError;
  return succeeded;
}

+ (BOOL)fileLoggingEnabled
{
  return g_fileLoggingEnabled.load(std::memory_order_relaxed);
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

+ (void)getLogArchiveWithCompletion:(LogArchiveCompletion)completion
{
  dispatch_queue_t utilityQueue = dispatch_get_global_queue(QOS_CLASS_UTILITY, 0);
  dispatch_async(utilityQueue, ^{
    NSError * error = nil;
    NSString * directoryPath = [self createTemporaryDirectoryWithError:&error];
    if (directoryPath == nil)
    {
      [self reportSystemMessage:error.localizedDescription type:OS_LOG_TYPE_ERROR];
      completion(nil, error);
      return;
    }

    // Only the snapshot runs on the file logging queue: it drains the writes that are already
    // submitted and excludes the later ones. Archiving and OSLog enumeration stay off this queue
    // so synchronous error records cannot be stuck behind them.
    dispatch_async([self fileLoggingQueue], ^{
      NSError * snapshotError = nil;
      NSArray<NSString *> * copies = [[self logger].fileWriter copyLogFilesToDirectory:directoryPath
                                                                                 error:&snapshotError];
      if (copies == nil)
      {
        [self reportSystemMessage:[NSString stringWithFormat:@"Failed to snapshot the diagnostic log: %@",
                                                             snapshotError.localizedDescription]
                             type:OS_LOG_TYPE_ERROR];
        copies = @[];
      }

      dispatch_async(utilityQueue, ^{
        NSMutableArray<NSString *> * reportFiles = [copies mutableCopy];
        NSError * systemLogError = nil;
        NSString * systemLogPath = [self writeOSLogStoreReportToDirectory:directoryPath error:&systemLogError];
        if (systemLogPath != nil)
          [reportFiles addObject:systemLogPath];
        else
          [self reportSystemMessage:systemLogError.localizedDescription type:OS_LOG_TYPE_ERROR];

        if (reportFiles.count == 0)
        {
          [self removeFileAtPath:directoryPath];
          NSError * error = snapshotError ?: systemLogError;
          completion(nil, error ?: MakeLoggerError(LoggerErrorNoLogs, @"No diagnostic logs are available."));
          return;
        }

        NSError * archiveError = nil;
        NSData * archiveData = [self archiveFiles:reportFiles inDirectory:directoryPath error:&archiveError];
        if (archiveData == nil)
          [self reportSystemMessage:archiveError.localizedDescription type:OS_LOG_TYPE_ERROR];
        completion(archiveData, archiveError);
      });
    });
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

+ (void)removeLegacyLogFile
{
  dispatch_async([self fileLoggingQueue], ^{ [self removeFileAtPath:kLegacyLogFilePath]; });
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

+ (BOOL)enableFileLoggingWithError:(NSError * __autoreleasing _Nullable * _Nullable)outError
{
  __block BOOL succeeded = YES;
  __block NSError * operationError = nil;
  [self runSyncOnFileLoggingQueue:^{
    if ([self fileLoggingEnabled])
      return;

    if (![[self logger].fileWriter openWithError:&operationError])
    {
      succeeded = NO;
      [self handleFileLoggingError:operationError];
      return;
    }
    ASSERT([self logger].fileWriter.isOpen, ());
    [self setFileLoggingState:YES error:nil];
  }];
  if (outError != nullptr)
    *outError = operationError;
  return succeeded;
}

+ (BOOL)disableFileLoggingWithError:(NSError * __autoreleasing _Nullable * _Nullable)outError
{
  __block BOOL succeeded = YES;
  __block NSError * operationError = nil;
  [self runSyncOnFileLoggingQueue:^{
    [self setFileLoggingState:NO error:nil];
    if (![[self logger].fileWriter closeAndRemoveFilesWithError:&operationError])
    {
      succeeded = NO;
      [self reportSystemMessage:operationError.localizedDescription type:OS_LOG_TYPE_ERROR];
    }
  }];
  if (outError != nullptr)
    *outError = operationError;
  return succeeded;
}

+ (void)setFileLoggingState:(BOOL)enabled error:(nullable NSError *)error
{
  dispatch_assert_queue([self fileLoggingQueue]);
  bool const changed = g_fileLoggingEnabled.exchange(enabled, std::memory_order_relaxed) != enabled;
  // Debug records are formatted only when a file can retain them. This mirrors Android's
  // LogsManager.nativeToggleCoreDebugLogs().
  base::g_LogLevel = enabled ? base::LDEBUG : base::GetDefaultLogLevel();
  if (!changed)
    return;

  dispatch_async(dispatch_get_main_queue(), ^{
    [NSNotificationCenter.defaultCenter postNotificationName:kFileLoggingStateDidChangeNotification object:error];
  });
}

+ (void)handleFileLoggingError:(NSError *)error
{
  dispatch_assert_queue([self fileLoggingQueue]);
  [self setFileLoggingState:NO error:error];

  // Stop writing, but keep whatever was collected: the failure can be transient (a full disk), and
  // those records are what the user enabled logging for. Only an explicit disable removes them.
  [[self logger].fileWriter closeWithError:nil];
  [self reportSystemMessage:[@"File logging failed: " stringByAppendingString:error.localizedDescription]
                       type:OS_LOG_TYPE_ERROR];
}

+ (void)logMessageWithLevel:(base::LogLevel)level src:(base::SrcPoint const &)src message:(std::string const &)message
{
  // Build the log message string.
  std::ostringstream output;
  base::LogHelper::WriteProlog(output, level);
  base::LogHelper::WriteLog(output, src, message);

  auto const logString = std::move(output).str();

  // Log the message into the system log. Oversized records are rare, so nothing is copied here
  // unless one has to be rebuilt with a truncation marker.
  std::string truncated;
  if (logString.size() > kMaxSystemLogRecordSize)
    truncated = TruncateForSystemLog(logString);
  os_log_with_type([self logger].osLogger, OSLogTypeFor(level), "%{public}s",
                   truncated.empty() ? logString.c_str() : truncated.c_str());

  if (!g_fileLoggingEnabled.load(std::memory_order_relaxed))
    return;

  // Copy the record once: blocks retain the NSData instead of copying the std::string again.
  NSData * data = [NSData dataWithBytes:logString.data() length:logString.size()];

  // Write errors synchronously: an error is often the last record before the process dies - through
  // the abort in LogMessage, or in the code that reported it - and that is exactly the record a
  // diagnostic log is collected for. Everything else goes asynchronously to keep the calling
  // (usually the main) thread out of the file I/O. The queue is serial, so the order is preserved.
  if (level >= base::LERROR)
    [self runSyncOnFileLoggingQueue:^{ [self tryWriteToFile:data]; }];
  else
    dispatch_async([self fileLoggingQueue], ^{ [self tryWriteToFile:data]; });
}

+ (void)tryWriteToFile:(NSData *)data
{
  dispatch_assert_queue([self fileLoggingQueue]);
  if (![self fileLoggingEnabled])
    return;
  ASSERT([self logger].fileWriter.isOpen, ());

  NSError * error = nil;
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
+ (nullable NSString *)createTemporaryDirectoryWithError:(NSError * __autoreleasing _Nullable * _Nullable)outError
{
  NSString * directoryPath = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  NSError * error = nil;
  if ([NSFileManager.defaultManager createDirectoryAtPath:directoryPath
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error])
    return directoryPath;

  if (outError != nullptr)
  {
    *outError = MakeLoggerError(
        LoggerErrorCreateTemporaryDirectory,
        [NSString stringWithFormat:@"Failed to create a temporary directory: %@", error.localizedDescription]);
  }
  return nil;
}

/// Archives |filePaths|, loads the result, and removes |directoryPath| on every return path.
+ (nullable NSData *)archiveFiles:(NSArray<NSString *> *)filePaths
                      inDirectory:(NSString *)directoryPath
                            error:(NSError * __autoreleasing _Nullable * _Nullable)outError
{
  ASSERT_GREATER(filePaths.count, 0, ());
  std::vector<std::string> files;
  files.reserve(filePaths.count);
  for (NSString * filePath in filePaths)
    files.emplace_back(filePath.UTF8String);

  NSData * archiveData = nil;
  NSString * archivePath = [directoryPath stringByAppendingPathComponent:kZipLogFileName];
  if (!CreateZipFromFiles(files, archivePath.UTF8String))
  {
    if (outError != nullptr)
      *outError = MakeLoggerError(LoggerErrorArchiveFailed, @"Failed to archive the diagnostic logs.");
  }
  else
  {
    NSError * error = nil;
    archiveData = [NSData dataWithContentsOfFile:archivePath options:0 error:&error];
    if (archiveData == nil && outError != nullptr)
    {
      *outError =
          MakeLoggerError(LoggerErrorReadArchiveFailed, [@"Failed to read the diagnostic archive: "
                                                            stringByAppendingString:error.localizedDescription]);
    }
  }

  [self removeFileAtPath:directoryPath];
  return archiveData;
}

/// Streams the system log into |directoryPath| instead of accumulating it in memory.
+ (nullable NSString *)writeOSLogStoreReportToDirectory:(NSString *)directoryPath
                                                  error:(NSError * __autoreleasing _Nullable * _Nullable)outError
{
  NSError * error = nil;
  OSLogStore * store = [OSLogStore storeWithScope:OSLogStoreCurrentProcessIdentifier error:&error];
  if (store == nil)
  {
    if (outError != nullptr)
      *outError = error ?: MakeLoggerError(LoggerErrorSystemLogUnavailable, @"The system log is unavailable.");
    return nil;
  }

  NSPredicate * predicate = [NSPredicate predicateWithFormat:@"subsystem == %@", kLoggerSubsystem];
  OSLogEnumerator * enumerator = [store entriesEnumeratorWithOptions:{} position:nil predicate:predicate error:&error];
  if (enumerator == nil)
  {
    if (outError != nullptr)
      *outError = error ?: MakeLoggerError(LoggerErrorSystemLogUnavailable, @"The system log is unavailable.");
    return nil;
  }

  NSString * reportFilePath = [directoryPath stringByAppendingPathComponent:kSystemLogFileName];
  bool isEmpty = true;
  try
  {
    // FileWriter buffers through stdio, so a report of tens of thousands of entries does not turn
    // into one write(2) per entry, and nothing is accumulated in memory.
    FileWriter reportFile(reportFilePath.UTF8String);
    for (id entry in enumerator)
    {
      if (![entry isKindOfClass:OSLogEntryLog.class])
        continue;
      isEmpty = false;
      reportFile << [entry composedMessage].UTF8String << "\n";
    }
  }
  catch (RootException const & e)
  {
    if (outError != nullptr)
    {
      *outError =
          MakeLoggerError(LoggerErrorSystemLogUnavailable,
                          [NSString stringWithFormat:@"Failed to write the system log report: %s", e.Msg().c_str()]);
    }
    return nil;
  }

  if (isEmpty)
  {
    if (outError != nullptr)
      *outError = MakeLoggerError(LoggerErrorNoLogs, @"The system log has no records to export.");
    return nil;
  }

  return reportFilePath;
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
