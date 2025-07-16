#import "Logger.h"
#import <OSLog/OSLog.h>

#include "base/logging.hpp"
#include "coding/zip_creator.hpp"

@interface Logger ()

@property(nullable, nonatomic) NSFileHandle * fileHandle;
@property(nonnull, nonatomic) os_log_t osLogger;
/// This property is introduced to avoid the CoreApi => Maps target dependency and stores the
/// MWMSettings.isFileLoggingEnabled value.
@property(class, nonatomic) BOOL fileLoggingEnabled;
@property(class, readonly, nonatomic) dispatch_queue_t fileLoggingQueue;

+ (Logger *)logger;
+ (void)enableFileLogging;
+ (void)disableFileLogging;
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

@implementation Logger

static BOOL _fileLoggingEnabled = NO;

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
  LOG(LINFO, ("File logging is enabled:", _fileLoggingEnabled ? "YES" : "NO"));
}

+ (BOOL)fileLoggingEnabled
{
  return _fileLoggingEnabled;
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
  Logger * logger = [self logger];
  return logger.fileHandle != nil ? [logger.fileHandle offsetInFile] : 0;
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

  _fileLoggingEnabled = YES;
}

+ (void)disableFileLogging
{
  Logger * logger = [self logger];

  [logger.fileHandle closeFile];
  logger.fileHandle = nil;
  [self removeFileAtPath:kLogFilePath];

  _fileLoggingEnabled = NO;
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

  // Log the message into the system log.
  os_log([self logger].osLogger, "%{public}s", logString.c_str());

  if (level < base::GetDefaultLogAbortLevel())
    dispatch_async([self fileLoggingQueue], ^{ [self tryWriteToFile:logString]; });
  else
    [self tryWriteToFile:logString];
}

+ (void)tryWriteToFile:(std::string const &)logString
{
  NSFileHandle * fileHandle = [self logger].fileHandle;
  if (fileHandle != nil)
  {
    [fileHandle seekToEndOfFile];
    [fileHandle writeData:[NSData dataWithBytes:logString.c_str() length:logString.length()]];
  }
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
