#import "LogFileWriter.h"

namespace
{
NSString * const kCurrentLogFileName = @"log.txt";
NSString * const kRotatedLogFileName = @"log.1.txt";
NSString * const kLogFileWriterErrorDomain = @"app.organicmaps.LogFileWriter";

typedef NS_ERROR_ENUM(kLogFileWriterErrorDomain, LogFileWriterError){
    LogFileWriterErrorInvalidCurrentFile = 1,
    LogFileWriterErrorOpenFailed,
    LogFileWriterErrorNotOpen,
};

BOOL SetError(NSError * __autoreleasing _Nullable * _Nullable error, LogFileWriterError code, NSString * description)
{
  if (error != nullptr)
    *error = [NSError errorWithDomain:kLogFileWriterErrorDomain
                                 code:code
                             userInfo:@{NSLocalizedDescriptionKey: description}];
  return NO;
}
}  // namespace

@interface LogFileWriter ()

@property(nonatomic) NSString * directoryPath;
@property(nonatomic) uint64_t maxFileSize;
@property(nonatomic) NSFileManager * fileManager;
@property(nullable, nonatomic) NSFileHandle * fileHandle;
@property(nonatomic) uint64_t currentFileSize;

@end

@implementation LogFileWriter

- (instancetype)initWithDirectoryPath:(NSString *)directoryPath maxFileSize:(uint64_t)maxFileSize
{
  return [self initWithDirectoryPath:directoryPath maxFileSize:maxFileSize fileManager:NSFileManager.defaultManager];
}

- (instancetype)initWithDirectoryPath:(NSString *)directoryPath
                          maxFileSize:(uint64_t)maxFileSize
                          fileManager:(NSFileManager *)fileManager
{
  NSParameterAssert(directoryPath.length > 0);
  NSParameterAssert(maxFileSize > 0);
  NSParameterAssert(fileManager != nil);
  self = [super init];
  if (self)
  {
    _directoryPath = [directoryPath copy];
    _maxFileSize = maxFileSize;
    _fileManager = fileManager;
    _currentFilePath = [_directoryPath stringByAppendingPathComponent:kCurrentLogFileName];
    _rotatedFilePath = [_directoryPath stringByAppendingPathComponent:kRotatedLogFileName];
  }
  return self;
}

- (void)dealloc
{
  [_fileHandle closeAndReturnError:nil];
}

- (BOOL)isOpen
{
  return self.fileHandle != nil;
}

- (BOOL)openWithError:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  if (self.isOpen)
    return YES;

  if (![self.fileManager createDirectoryAtPath:self.directoryPath
                   withIntermediateDirectories:YES
                                    attributes:nil
                                         error:error])
    return NO;

  // A diagnostic log can contain locations, so it must never reach a backup. Set on every open, so
  // that a directory recreated behind our back does not silently lose the flag.
  NSURL * directoryURL = [NSURL fileURLWithPath:self.directoryPath isDirectory:YES];
  if (![directoryURL setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:error])
    return NO;

  BOOL isDirectory = NO;
  BOOL const exists = [self.fileManager fileExistsAtPath:self.currentFilePath isDirectory:&isDirectory];
  if (exists && isDirectory)
    return SetError(error, LogFileWriterErrorInvalidCurrentFile, @"The current log path is a directory.");

  if (!exists && ![self.fileManager createFileAtPath:self.currentFilePath contents:nil attributes:nil])
    return SetError(error, LogFileWriterErrorOpenFailed, @"Failed to create the current log file.");

  NSFileHandle * fileHandle = [NSFileHandle fileHandleForWritingAtPath:self.currentFilePath];
  if (fileHandle == nil)
    return SetError(error, LogFileWriterErrorOpenFailed, @"Failed to open the current log file.");

  unsigned long long offset = 0;
  if (![fileHandle seekToEndReturningOffset:&offset error:error])
  {
    [fileHandle closeAndReturnError:nil];
    return NO;
  }

  self.fileHandle = fileHandle;
  self.currentFileSize = offset;
  return YES;
}

- (BOOL)writeData:(NSData *)data error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  if (!self.isOpen)
    return SetError(error, LogFileWriterErrorNotOpen, @"The log file is not open.");

  uint64_t const dataSize = data.length;
  if (self.currentFileSize > 0 && self.currentFileSize + dataSize > self.maxFileSize && ![self rotateWithError:error])
    return NO;

  if (![self.fileHandle writeData:data error:error])
    return NO;

  self.currentFileSize += dataSize;
  return YES;
}

- (BOOL)closeWithError:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  NSFileHandle * fileHandle = self.fileHandle;
  self.fileHandle = nil;
  self.currentFileSize = 0;
  return fileHandle == nil || [fileHandle closeAndReturnError:error];
}

- (BOOL)closeAndRemoveFilesWithError:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  NSError * firstError = nil;
  // Continue removing the files after a close error so an explicit disable leaves no diagnostic data behind.
  [self closeWithError:&firstError];

  for (NSString * filePath in @[self.currentFilePath, self.rotatedFilePath])
  {
    if (![self.fileManager fileExistsAtPath:filePath])
      continue;

    NSError * removeError = nil;
    if (![self.fileManager removeItemAtPath:filePath error:&removeError] && firstError == nil)
      firstError = removeError;
  }

  if (error != nullptr)
    *error = firstError;
  return firstError == nil;
}

- (nullable NSArray<NSString *> *)copyLogFilesToDirectory:(NSString *)directoryPath
                                                    error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  // The rotated file is optional, the current one is required.
  NSArray<NSString *> * sourcePaths = [self.fileManager fileExistsAtPath:self.rotatedFilePath]
                                        ? @[self.rotatedFilePath, self.currentFilePath]
                                        : @[self.currentFilePath];
  NSMutableArray<NSString *> * copies = [NSMutableArray arrayWithCapacity:sourcePaths.count];
  for (NSString * sourcePath in sourcePaths)
  {
    NSString * destinationPath = [directoryPath stringByAppendingPathComponent:sourcePath.lastPathComponent];
    if (![self.fileManager copyItemAtPath:sourcePath toPath:destinationPath error:error])
      return nil;
    [copies addObject:destinationPath];
  }
  return copies;
}

- (uint64_t)totalFileSize
{
  uint64_t totalSize = 0;
  for (NSString * filePath in @[self.currentFilePath, self.rotatedFilePath])
  {
    NSDictionary<NSFileAttributeKey, id> * attributes = [self.fileManager attributesOfItemAtPath:filePath error:nil];
    totalSize += attributes.fileSize;
  }
  return totalSize;
}

- (BOOL)rotateWithError:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  if (![self closeWithError:error])
    return NO;

  if ([self.fileManager fileExistsAtPath:self.rotatedFilePath] &&
      ![self.fileManager removeItemAtPath:self.rotatedFilePath error:error])
    return NO;

  if (![self.fileManager moveItemAtPath:self.currentFilePath toPath:self.rotatedFilePath error:error])
    return NO;

  // The current file is gone after the move, and opening recreates it.
  return [self openWithError:error];
}

@end
