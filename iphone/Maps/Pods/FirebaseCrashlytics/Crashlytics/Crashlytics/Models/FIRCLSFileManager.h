// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>

@class FIRCLSInternalReport;

@interface FIRCLSFileManager : NSObject

- (instancetype)init NS_DESIGNATED_INITIALIZER;

@property(nonatomic, readonly) NSFileManager *underlyingFileManager;

/**
 * Returns the folder containing the settings file
 */
@property(nonatomic, readonly) NSString *settingsDirectoryPath;

/**
 * Returns the path to the settings file
 */
@property(nonatomic, readonly) NSString *settingsFilePath;

/**
 * Path to the file that holds the ttl and keys that invalidate settings
 */
@property(nonatomic, readonly) NSString *settingsCacheKeyPath;

@property(nonatomic, readonly) NSString *rootPath;
@property(nonatomic, readonly) NSString *structurePath;
@property(nonatomic, readonly) NSString *activePath;
@property(nonatomic, readonly) NSString *processingPath;
@property(nonatomic, readonly) NSString *pendingPath;
@property(nonatomic, readonly) NSString *preparedPath;
@property(nonatomic, readonly) NSString *legacyPreparedPath;
@property(nonatomic, readonly) NSArray *activePathContents;
@property(nonatomic, readonly) NSArray *legacyPreparedPathContents;
@property(nonatomic, readonly) NSArray *preparedPathContents;
@property(nonatomic, readonly) NSArray *processingPathContents;

- (BOOL)fileExistsAtPath:(NSString *)path;
- (BOOL)createFileAtPath:(NSString *)path
                contents:(NSData *)data
              attributes:(NSDictionary<NSFileAttributeKey, id> *)attr;
- (BOOL)createDirectoryAtPath:(NSString *)path;
- (BOOL)removeItemAtPath:(NSString *)path;
- (BOOL)removeContentsOfDirectoryAtPath:(NSString *)path;
- (BOOL)moveItemAtPath:(NSString *)path toDirectory:(NSString *)destDir;
- (void)enumerateFilesInDirectory:(NSString *)directory
                       usingBlock:(void (^)(NSString *filePath, NSString *extension))block;
- (NSNumber *)fileSizeAtPath:(NSString *)path;
- (NSArray *)contentsOfDirectory:(NSString *)path;

// logic of managing files/directories
- (BOOL)createReportDirectories;
- (NSString *)setupNewPathForExecutionIdentifier:(NSString *)identifier;

- (BOOL)moveItemAtPath:(NSString *)srcPath toPath:(NSString *)dstPath error:(NSError **)error;

- (NSData *)dataWithContentsOfFile:(NSString *)path;

@end
