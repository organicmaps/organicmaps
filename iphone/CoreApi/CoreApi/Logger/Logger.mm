#import "Logger.h"
#import <OSLog/OSLog.h>

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "coding/zip_creator.hpp"

#include <atomic>
#include <string>
#include <vector>

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
+ (nullable NSFileHandle *)rotateLogFile;
+ (nullable NSURL *)zipFiles:(NSArray<NSString *> *)filePaths;
+ (nullable NSURL *)zipOSLogStoreReport;
+ (void)removeFileAtPath:(NSString *)filePath;
+ (base::LogLevel)baseLevel:(LogLevel)level;

@end

// Subsystem and category are used for the OSLog.
NSString * const kLoggerSubsystem = [[NSBundle mainBundle] bundleIdentifier];
NSString * const kLoggerCategory = @"OM";
NSString * const kLogFileName = @"log.txt";
NSString * const kRotatedLogFileName = @"log.1.txt";
NSString * const kZipLogFileName = @"log.zip";

// Diagnostic logs are temporary support data. Caches keeps them out of backups (QA1719) and, unlike
// Documents, out of the Files app, which the app opens up with UIFileSharingEnabled.
NSString * const kLogDirectoryPath = [[NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:@"Logs"];
NSString * const kLogFilePath = [kLogDirectoryPath stringByAppendingPathComponent:kLogFileName];
NSString * const kRotatedLogFilePath = [kLogDirectoryPath stringByAppendingPathComponent:kRotatedLogFileName];
NSString * const kLegacyLogFilePath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
    firstObject] stringByAppendingPathComponent:kLogFileName];

// The current file is rotated once, so at most two of these are kept. The previous 100 MB cap was
// never enforced during a session, and a report of that size is not attachable to an email anyway.
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
    // Diagnostic logs used to be kept in Documents, where the Files app shows them to the user and
    // where backups pick them up. Drop whatever an older version left behind.
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
    _osLogger = os_log_create(kLoggerSubsystem.UTF8String, kLoggerCategory.UTF8String);
  return self;
}

// MARK: - Public

+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled
{
  fileLoggingEnabled ? [self enableFileLogging] : [self disableFileLogging];
  // Debug records are worth formatting only when there is a file to keep them in, so derive the
  // level from the state the logger ended up in, not from the requested one. Mirrors Android's
  // LogsManager.nativeToggleCoreDebugLogs().
  base::g_LogLevel = [self fileLoggingEnabled] ? base::LDEBUG : base::GetDefaultLogLevel();
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

+ (void)getLogFileURLWithCompletion:(void (^)(NSURL * _Nullable logFileURL))completion
{
  if ([self fileLoggingEnabled])
  {
    // Running on the file logging queue is the snapshot: it drains the writes that are already
    // submitted and excludes the later ones for as long as the archive is being created. Nothing
    // waits for it, because ordinary writes are asynchronous.
    dispatch_async([self fileLoggingQueue], ^{
      [[self logger].fileHandle synchronizeAndReturnError:nil];
      NSMutableArray<NSString *> * filePaths = [NSMutableArray array];
      // The rotated file goes first, so that the archive reads in chronological order.
      for (NSString * filePath in @[kRotatedLogFilePath, kLogFilePath])
        if ([NSFileManager.defaultManager fileExistsAtPath:filePath])
          [filePaths addObject:filePath];
      completion([self zipFiles:filePaths]);
    });
  }
  else
  {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ completion([self zipOSLogStoreReport]); });
  }
}

+ (uint64_t)getLogFileSize
{
  auto const fileSize = [](NSString * filePath)
  {
    NSDictionary * attributes = [NSFileManager.defaultManager attributesOfItemAtPath:filePath error:nil];
    return attributes != nil ? attributes.fileSize : 0;
  };
  return fileSize(kLogFilePath) + fileSize(kRotatedLogFilePath);
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

    NSError * error;
    if (![fileManager createDirectoryAtPath:kLogDirectoryPath
                withIntermediateDirectories:YES
                                 attributes:nil
                                      error:&error])
    {
      LOG(LERROR,
          ("Failed to create the log directory", kLogDirectoryPath.UTF8String, error.localizedDescription.UTF8String));
      [self disableFileLogging];
      return;
    }

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

    logger.fileHandle = fileHandle;

    _fileLoggingEnabled.store(true, std::memory_order_relaxed);
  }];
}

+ (void)disableFileLogging
{
  [self runSyncOnFileLoggingQueue:^{
    Logger * logger = [self logger];

    _fileLoggingEnabled.store(false, std::memory_order_relaxed);

    [logger.fileHandle closeAndReturnError:nil];
    logger.fileHandle = nil;
    [self removeFileAtPath:kLogFilePath];
    [self removeFileAtPath:kRotatedLogFilePath];
  }];
}

+ (void)logMessageWithLevel:(base::LogLevel)level src:(base::SrcPoint const &)src message:(std::string const &)message
{
  // Build the log message string.
  auto & logHelper = base::LogHelper::Instance();
  std::ostringstream output;
  // TODO: (KK) Either guard this call, or refactor thread ids in logHelper.
  logHelper.WriteProlog(output, level);
  logHelper.WriteLog(output, src, message);

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

  // Write errors synchronously to capture them before a possible abort, everything else
  // asynchronously to keep the calling (usually the main) thread out of the file I/O.
  // The queue is serial, so the order of the records is preserved either way.
  if (level >= base::LERROR)
    [self runSyncOnFileLoggingQueue:^{ [self tryWriteToFile:logString]; }];
  else
    dispatch_async([self fileLoggingQueue], ^{ [self tryWriteToFile:logString]; });
}

+ (void)tryWriteToFile:(std::string const &)logString
{
  dispatch_assert_queue([self fileLoggingQueue]);
  NSFileHandle * fileHandle = [self logger].fileHandle;
  if (fileHandle == nil)
    return;

  // Never log from here: logging from the log writer would recurse on every failed write.
  unsigned long long offset = 0;
  if (![fileHandle seekToEndReturningOffset:&offset error:nil])
    return;

  // Rotate before the record, never in the middle of one, so a record is never split. A single
  // oversized record is the only way a file can end up above the cap.
  if (offset > 0 && offset + logString.size() > kMaxLogFileSize)
  {
    fileHandle = [self rotateLogFile];
    if (fileHandle == nil)
      return;
  }

  [fileHandle writeData:[NSData dataWithBytes:logString.data() length:logString.size()] error:nil];
}

/// Moves the current log file aside, replacing the previously rotated one, and starts a new file.
/// @return The handle of the new file, or nil if it cannot be opened.
+ (nullable NSFileHandle *)rotateLogFile
{
  dispatch_assert_queue([self fileLoggingQueue]);
  Logger * logger = [self logger];
  NSFileManager * fileManager = NSFileManager.defaultManager;

  [logger.fileHandle closeAndReturnError:nil];
  logger.fileHandle = nil;

  [fileManager removeItemAtPath:kRotatedLogFilePath error:nil];
  // If the file cannot be moved aside it is restarted instead: the size cap has to hold either way.
  [fileManager moveItemAtPath:kLogFilePath toPath:kRotatedLogFilePath error:nil];
  [fileManager createFileAtPath:kLogFilePath contents:nil attributes:nil];

  logger.fileHandle = [NSFileHandle fileHandleForWritingAtPath:kLogFilePath];
  if (logger.fileHandle == nil)
    _fileLoggingEnabled.store(false, std::memory_order_relaxed);
  return logger.fileHandle;
}

+ (void)runSyncOnFileLoggingQueue:(dispatch_block_t)block
{
  if (dispatch_get_specific(kFileLoggingQueueKey) != nullptr)
    block();
  else
    dispatch_sync([self fileLoggingQueue], block);
}

/// Archives |filePaths| into a uniquely named temporary directory, so that repeated or concurrent
/// reports cannot overwrite or delete each other's archive. The caller owns the result and is
/// expected to remove its directory.
+ (nullable NSURL *)zipFiles:(NSArray<NSString *> *)filePaths
{
  if (filePaths.count == 0)
  {
    LOG(LWARNING, ("There are no log files to export."));
    return nil;
  }

  NSString * directoryPath = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  NSError * error;
  if (![NSFileManager.defaultManager createDirectoryAtPath:directoryPath
                               withIntermediateDirectories:YES
                                                attributes:nil
                                                     error:&error])
  {
    LOG(LERROR, ("Failed to create a temporary directory for the log archive:", error.localizedDescription.UTF8String));
    return nil;
  }

  std::vector<std::string> files;
  files.reserve(filePaths.count);
  for (NSString * filePath in filePaths)
    files.emplace_back(filePath.UTF8String);

  NSString * zipFilePath = [directoryPath stringByAppendingPathComponent:kZipLogFileName];
  if (!CreateZipFromFiles(files, zipFilePath.UTF8String))
  {
    LOG(LERROR, ("Failed to zip the log files."));
    [self removeFileAtPath:directoryPath];
    return nil;
  }
  return [NSURL fileURLWithPath:zipFilePath];
}

/// Rebuilds a report from the system log for users who have not enabled file logging. The entries
/// are streamed into a file one by one instead of being accumulated in memory first.
+ (nullable NSURL *)zipOSLogStoreReport
{
  NSError * error;
  OSLogStore * store = [OSLogStore storeWithScope:OSLogStoreCurrentProcessIdentifier error:&error];
  if (store == nil)
  {
    LOG(LERROR, (error.localizedDescription.UTF8String));
    return nil;
  }

  NSPredicate * predicate = [NSPredicate predicateWithFormat:@"subsystem == %@", kLoggerSubsystem];
  OSLogEnumerator * enumerator = [store entriesEnumeratorWithOptions:{} position:nil predicate:predicate error:&error];
  if (enumerator == nil)
  {
    LOG(LERROR, (error.localizedDescription.UTF8String));
    return nil;
  }

  NSString * directoryPath = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  if (![NSFileManager.defaultManager createDirectoryAtPath:directoryPath
                               withIntermediateDirectories:YES
                                                attributes:nil
                                                     error:&error])
  {
    LOG(LERROR, ("Failed to create a temporary directory for the report:", error.localizedDescription.UTF8String));
    return nil;
  }

  // Never the persistent log path: a later file logging session would append to a stale report.
  NSString * reportFilePath = [directoryPath stringByAppendingPathComponent:kLogFileName];
  [NSFileManager.defaultManager createFileAtPath:reportFilePath contents:nil attributes:nil];
  NSFileHandle * reportFile = [NSFileHandle fileHandleForWritingAtPath:reportFilePath];
  if (reportFile == nil)
  {
    LOG(LERROR, ("Failed to open the report file for writing:", reportFilePath.UTF8String));
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  BOOL isEmpty = YES;
  for (id entry in enumerator)
  {
    if (![entry isKindOfClass:OSLogEntryLog.class])
      continue;
    NSString * line = [[entry composedMessage] stringByAppendingString:@"\n"];
    [reportFile writeData:[line dataUsingEncoding:NSUTF8StringEncoding] error:nil];
    isEmpty = NO;
  }
  [reportFile closeAndReturnError:nil];

  if (isEmpty)
  {
    LOG(LINFO, ("OSLog entry is empty."));
    [self removeFileAtPath:directoryPath];
    return nil;
  }

  NSURL * zipFileURL = [self zipFiles:@[reportFilePath]];
  // The archive is created in its own directory, so the uncompressed report is not needed anymore.
  [self removeFileAtPath:directoryPath];
  return zipFileURL;
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
