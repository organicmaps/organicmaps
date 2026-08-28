#import <XCTest/XCTest.h>

#import <CoreApi/Logger.h>
#import "../../../CoreApi/CoreApi/Logger/LogFileWriter.h"

#include "coding/zip_reader.hpp"

@interface Logger (Tests)

+ (Logger *)logger;

@end

@interface FailingFileManager : NSFileManager

@property(nonatomic) BOOL failMove;
@property(nonatomic) BOOL failCreate;
@property(nullable, nonatomic) NSString * failCopyFileName;

@end

@implementation FailingFileManager

- (BOOL)copyItemAtPath:(NSString *)sourcePath
                toPath:(NSString *)destinationPath
                 error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  if (![sourcePath.lastPathComponent isEqualToString:self.failCopyFileName])
    return [super copyItemAtPath:sourcePath toPath:destinationPath error:error];

  if (error != nullptr)
    *error = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadNoPermissionError userInfo:nil];
  return NO;
}

- (BOOL)createFileAtPath:(NSString *)path contents:(NSData *)data attributes:(NSDictionary *)attributes
{
  return self.failCreate ? NO : [super createFileAtPath:path contents:data attributes:attributes];
}

- (BOOL)moveItemAtPath:(NSString *)sourcePath
                toPath:(NSString *)destinationPath
                 error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  if (!self.failMove)
    return [super moveItemAtPath:sourcePath toPath:destinationPath error:error];

  if (error != nullptr)
  {
    *error = [NSError errorWithDomain:NSCocoaErrorDomain
                                 code:NSFileWriteUnknownError
                             userInfo:@{NSLocalizedDescriptionKey: @"Injected move failure"}];
  }
  return NO;
}

@end

@interface FailingCloseLogFileWriter : LogFileWriter

@end

@implementation FailingCloseLogFileWriter

- (BOOL)closeWithError:(NSError * __autoreleasing _Nullable * _Nullable)error
{
  [super closeWithError:nil];
  if (error != nullptr)
    *error = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileWriteUnknownError userInfo:nil];
  return NO;
}

@end

@interface LogFileWriterTests : XCTestCase

@property(nonatomic) NSString * directoryPath;

@end

@implementation LogFileWriterTests

- (void)setUp
{
  [super setUp];
  self.directoryPath = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
}

- (void)tearDown
{
  [NSFileManager.defaultManager removeItemAtPath:self.directoryPath error:nil];
  [super tearDown];
}

- (void)testWritesAppendToTheCurrentFile
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:8];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"abc"] error:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"def"] error:&error], @"%@", error);

  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"abcdef"]);
  XCTAssertEqual(writer.totalFileSize, 6);
}

- (void)testReopeningAppendsToTheExistingFile
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:8];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"abc"] error:&error], @"%@", error);
  XCTAssertTrue([writer closeWithError:&error], @"%@", error);

  writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:8];
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"def"] error:&error], @"%@", error);

  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"abcdef"]);
}

- (void)testRotationKeepsRecordsWholeAndReplacesTheOlderFile
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:8];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"123456"] error:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"abcdef"] error:&error], @"%@", error);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.rotatedFilePath], [self data:@"123456"]);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"abcdef"]);

  XCTAssertTrue([writer writeData:[self data:@"XY"] error:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"Z"] error:&error], @"%@", error);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.rotatedFilePath], [self data:@"abcdefXY"]);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"Z"]);
}

- (void)testOversizedRecordIsNotSplit
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:4];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"123456"] error:&error], @"%@", error);

  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"123456"]);
  XCTAssertFalse([NSFileManager.defaultManager fileExistsAtPath:writer.rotatedFilePath]);
}

- (void)testOpeningRejectsADirectoryAtTheCurrentFilePath
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:4];
  NSError * error = nil;
  XCTAssertTrue([NSFileManager.defaultManager createDirectoryAtPath:writer.currentFilePath
                                        withIntermediateDirectories:YES
                                                         attributes:nil
                                                              error:&error],
                @"%@", error);
  XCTAssertFalse([writer openWithError:&error]);
  XCTAssertNotNil(error);
  XCTAssertFalse(writer.isOpen);
}

- (void)testDisablingRemovesBothFilesEvenAfterACloseError
{
  for (Class writerClass in @[LogFileWriter.class, FailingCloseLogFileWriter.class])
  {
    LogFileWriter * writer = [[writerClass alloc] initWithDirectoryPath:self.directoryPath maxFileSize:4];
    NSError * error = nil;
    XCTAssertTrue([writer openWithError:&error], @"%@", error);
    XCTAssertTrue([writer writeData:[self data:@"1234"] error:&error], @"%@", error);
    // Populate the rotated file without invoking the injected close failure during rotation.
    XCTAssertTrue([[self data:@"older"] writeToFile:writer.rotatedFilePath options:0 error:&error], @"%@", error);

    BOOL const expectsSuccess = writerClass == LogFileWriter.class;
    XCTAssertEqual([writer closeAndRemoveFilesWithError:&error], expectsSuccess);
    XCTAssertEqual(error == nil, expectsSuccess);
    XCTAssertFalse(writer.isOpen);
    XCTAssertFalse([NSFileManager.defaultManager fileExistsAtPath:writer.currentFilePath]);
    XCTAssertFalse([NSFileManager.defaultManager fileExistsAtPath:writer.rotatedFilePath]);
  }
}

- (void)testMoveFailureStopsTheWriterWithoutTruncatingTheCurrentFile
{
  FailingFileManager * fileManager = [[FailingFileManager alloc] init];
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath
                                                            maxFileSize:4
                                                            fileManager:fileManager];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"1234"] error:&error], @"%@", error);

  fileManager.failMove = YES;
  XCTAssertFalse([writer writeData:[self data:@"5"] error:&error]);
  XCTAssertNotNil(error);
  XCTAssertFalse(writer.isOpen);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:writer.currentFilePath], [self data:@"1234"]);
  XCTAssertFalse([fileManager fileExistsAtPath:writer.rotatedFilePath]);
}

- (void)testSnapshotAcceptsRotatedFileAfterReopenFailure
{
  FailingFileManager * fileManager = [[FailingFileManager alloc] init];
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath
                                                            maxFileSize:4
                                                            fileManager:fileManager];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"1234"] error:&error], @"%@", error);

  fileManager.failCreate = YES;
  XCTAssertFalse([writer writeData:[self data:@"5"] error:&error]);
  XCTAssertFalse(writer.isOpen);
  XCTAssertFalse([fileManager fileExistsAtPath:writer.currentFilePath]);
  XCTAssertTrue([fileManager fileExistsAtPath:writer.rotatedFilePath]);

  NSString * snapshotPath = [self snapshotDirectory];
  NSArray<NSString *> * copies = [writer copyLogFilesToDirectory:snapshotPath error:&error];
  XCTAssertEqual(copies.count, 1);
  XCTAssertEqualObjects(copies.firstObject.lastPathComponent, writer.rotatedFilePath.lastPathComponent);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.firstObject], [self data:@"1234"]);
}

- (void)testSnapshotIsChronologicalAndReturnsEveryExistingFile
{
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath maxFileSize:4];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"1234"] error:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"5"] error:&error], @"%@", error);

  NSString * snapshotPath = [self snapshotDirectory];
  NSArray<NSString *> * copies = [writer copyLogFilesToDirectory:snapshotPath error:&error];
  XCTAssertEqual(copies.count, 2);
  XCTAssertEqualObjects(copies.firstObject.lastPathComponent, writer.rotatedFilePath.lastPathComponent);
  XCTAssertEqualObjects(copies.lastObject.lastPathComponent, writer.currentFilePath.lastPathComponent);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.firstObject], [self data:@"1234"]);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.lastObject], [self data:@"5"]);

  XCTAssertTrue([NSFileManager.defaultManager removeItemAtPath:writer.currentFilePath error:&error], @"%@", error);
  NSString * secondSnapshotPath = [self snapshotDirectory];
  copies = [writer copyLogFilesToDirectory:secondSnapshotPath error:&error];
  XCTAssertEqual(copies.count, 1);
  XCTAssertEqualObjects(copies.firstObject.lastPathComponent, writer.rotatedFilePath.lastPathComponent);

  XCTAssertTrue([NSFileManager.defaultManager removeItemAtPath:writer.rotatedFilePath error:&error], @"%@", error);
  NSString * emptySnapshotPath = [self snapshotDirectory];
  copies = [writer copyLogFilesToDirectory:emptySnapshotPath error:&error];
  XCTAssertEqual(copies.count, 0);
}

- (void)testSnapshotKeepsSuccessfulCopiesWhenEitherFileFails
{
  FailingFileManager * fileManager = [[FailingFileManager alloc] init];
  LogFileWriter * writer = [[LogFileWriter alloc] initWithDirectoryPath:self.directoryPath
                                                            maxFileSize:4
                                                            fileManager:fileManager];
  NSError * error = nil;
  XCTAssertTrue([writer openWithError:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"1234"] error:&error], @"%@", error);
  XCTAssertTrue([writer writeData:[self data:@"5"] error:&error], @"%@", error);

  for (NSString * failedPath in @[writer.currentFilePath, writer.rotatedFilePath])
  {
    fileManager.failCopyFileName = failedPath.lastPathComponent;
    NSArray<NSString *> * copies = [writer copyLogFilesToDirectory:[self snapshotDirectory] error:&error];
    XCTAssertNotNil(error);
    XCTAssertEqual(copies.count, 1);
    NSString * successfulPath =
        [failedPath isEqualToString:writer.currentFilePath] ? writer.rotatedFilePath : writer.currentFilePath;
    XCTAssertEqualObjects(copies.firstObject.lastPathComponent, successfulPath.lastPathComponent);
    XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.firstObject],
                          [NSData dataWithContentsOfFile:successfulPath]);
  }
}

- (void)testArchiveKeepsPartialSnapshotAndExplainsTheMissingFile
{
  BOOL const wasEnabled = Logger.fileLoggingEnabled;
  Logger.fileLoggingEnabled = NO;
  Logger.fileLoggingEnabled = YES;
  LogFileWriter * writer = [[Logger logger] valueForKey:@"fileWriter"];
  NSFileManager * originalFileManager = [writer valueForKey:@"fileManager"];
  FailingFileManager * fileManager = [[FailingFileManager alloc] init];
  fileManager.failCopyFileName = writer.currentFilePath.lastPathComponent;
  @try
  {
    [Logger flushWithCompletion:^{
      XCTAssertTrue([[self data:@"Earlier diagnostic records"] writeToFile:writer.rotatedFilePath atomically:NO]);
      [writer setValue:fileManager forKey:@"fileManager"];
    }];

    XCTestExpectation * exported = [self expectationWithDescription:@"A partial diagnostic report is created"];
    __block NSData * archiveData = nil;
    [Logger getLogArchiveWithCompletion:^(NSData * data, NSError * error) {
      XCTAssertFalse(NSThread.isMainThread);
      XCTAssertNil(error);
      archiveData = data;
      [exported fulfill];
    }];
    [self waitForExpectations:@[exported] timeout:30];
    XCTAssertGreaterThan(archiveData.length, 0);

    NSString * archivePath = [[self snapshotDirectory] stringByAppendingPathComponent:@"report.zip"];
    XCTAssertTrue([archiveData writeToFile:archivePath atomically:NO]);
    ZipFileReader::FileList entries;
    ZipFileReader::FilesList(archivePath.UTF8String, entries);
    NSMutableSet<NSString *> * names = [NSMutableSet set];
    for (auto const & entry : entries)
      [names addObject:@(entry.first.c_str())];
    XCTAssertTrue([names containsObject:@"log.1.txt"]);
    XCTAssertTrue([names containsObject:@"export-errors.txt"]);
    XCTAssertFalse([names containsObject:@"log.txt"]);

    NSString * errorPath = [self.directoryPath stringByAppendingPathComponent:@"export-errors.txt"];
    ZipFileReader::UnzipFile(archivePath.UTF8String, "export-errors.txt", errorPath.UTF8String);
    NSString * details = [NSString stringWithContentsOfFile:errorPath encoding:NSUTF8StringEncoding error:nil];
    XCTAssertTrue([details containsString:@"Failed to snapshot the diagnostic log:"]);
  }
  @finally
  {
    [Logger flushWithCompletion:^{ [writer setValue:originalFileManager forKey:@"fileManager"]; }];
    [Logger flushSynchronously];
    Logger.fileLoggingEnabled = wasEnabled;
  }
}

- (NSString *)snapshotDirectory
{
  NSString * path = [self.directoryPath stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  NSError * error = nil;
  XCTAssertTrue([NSFileManager.defaultManager createDirectoryAtPath:path
                                        withIntermediateDirectories:YES
                                                         attributes:nil
                                                              error:&error],
                @"%@", error);
  return path;
}

- (NSData *)data:(NSString *)string
{
  return [string dataUsingEncoding:NSUTF8StringEncoding];
}

@end
