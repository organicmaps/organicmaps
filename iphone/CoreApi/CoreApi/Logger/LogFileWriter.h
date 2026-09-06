#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Manages the current and rotated diagnostic log files. Its caller is responsible for invoking
/// every method except totalFileSize from one queue.
@interface LogFileWriter : NSObject

@property(nonatomic, readonly) NSString * currentFilePath;
@property(nonatomic, readonly) NSString * rotatedFilePath;
@property(nonatomic, readonly, getter=isOpen) BOOL open;

/// |maxFileSize| is the per-file cap. One rotated file may also be retained.
- (instancetype)initWithDirectoryPath:(NSString *)directoryPath maxFileSize:(uint64_t)maxFileSize;
- (instancetype)initWithDirectoryPath:(NSString *)directoryPath
                          maxFileSize:(uint64_t)maxFileSize
                          fileManager:(NSFileManager *)fileManager NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

- (BOOL)openWithError:(NSError * __autoreleasing _Nullable * _Nullable)error;
- (BOOL)writeData:(NSData *)data error:(NSError * __autoreleasing _Nullable * _Nullable)error;
- (BOOL)closeWithError:(NSError * __autoreleasing _Nullable * _Nullable)error;
- (BOOL)closeAndRemoveFilesWithError:(NSError * __autoreleasing _Nullable * _Nullable)error;

/// Copies every existing log file, with the rotated file first. Returns an empty array when no log
/// file exists. The current file may be absent after a failed rotation reopen.
- (nullable NSArray<NSString *> *)copyLogFilesToDirectory:(NSString *)directoryPath
                                                    error:(NSError * __autoreleasing _Nullable * _Nullable)error;

/// Does not access queue-confined in-memory state, so callers may use it from any queue.
- (uint64_t)totalFileSize;

@end

NS_ASSUME_NONNULL_END
