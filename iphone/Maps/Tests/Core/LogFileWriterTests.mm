#import <XCTest/XCTest.h>

#import "LogFileWriter.h"

@interface FailingFileManager : NSFileManager

@property(nonatomic) BOOL failMove;
@property(nonatomic) BOOL failCreate;

@end

@implementation FailingFileManager

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

  NSString * snapshotPath = [self.directoryPath stringByAppendingPathComponent:@"snapshot"];
  XCTAssertTrue([fileManager createDirectoryAtPath:snapshotPath
                       withIntermediateDirectories:NO
                                        attributes:nil
                                             error:&error],
                @"%@", error);
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

  NSString * snapshotPath = [self.directoryPath stringByAppendingPathComponent:@"snapshot"];
  XCTAssertTrue([NSFileManager.defaultManager createDirectoryAtPath:snapshotPath
                                        withIntermediateDirectories:NO
                                                         attributes:nil
                                                              error:&error],
                @"%@", error);
  NSArray<NSString *> * copies = [writer copyLogFilesToDirectory:snapshotPath error:&error];
  XCTAssertEqual(copies.count, 2);
  XCTAssertEqualObjects(copies.firstObject.lastPathComponent, writer.rotatedFilePath.lastPathComponent);
  XCTAssertEqualObjects(copies.lastObject.lastPathComponent, writer.currentFilePath.lastPathComponent);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.firstObject], [self data:@"1234"]);
  XCTAssertEqualObjects([NSData dataWithContentsOfFile:copies.lastObject], [self data:@"5"]);

  XCTAssertTrue([NSFileManager.defaultManager removeItemAtPath:writer.currentFilePath error:&error], @"%@", error);
  NSString * secondSnapshotPath = [self.directoryPath stringByAppendingPathComponent:@"second-snapshot"];
  XCTAssertTrue([NSFileManager.defaultManager createDirectoryAtPath:secondSnapshotPath
                                        withIntermediateDirectories:NO
                                                         attributes:nil
                                                              error:&error],
                @"%@", error);
  copies = [writer copyLogFilesToDirectory:secondSnapshotPath error:&error];
  XCTAssertEqual(copies.count, 1);
  XCTAssertEqualObjects(copies.firstObject.lastPathComponent, writer.rotatedFilePath.lastPathComponent);

  XCTAssertTrue([NSFileManager.defaultManager removeItemAtPath:writer.rotatedFilePath error:&error], @"%@", error);
  NSString * emptySnapshotPath = [self.directoryPath stringByAppendingPathComponent:@"empty-snapshot"];
  XCTAssertTrue([NSFileManager.defaultManager createDirectoryAtPath:emptySnapshotPath
                                        withIntermediateDirectories:NO
                                                         attributes:nil
                                                              error:&error],
                @"%@", error);
  copies = [writer copyLogFilesToDirectory:emptySnapshotPath error:&error];
  XCTAssertEqual(copies.count, 0);
}

- (NSData *)data:(NSString *)string
{
  return [string dataUsingEncoding:NSUTF8StringEncoding];
}

@end
