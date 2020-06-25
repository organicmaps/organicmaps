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

#import "FIRCLSFileManager.h"

#import "FIRCLSApplication.h"
#import "FIRCLSInternalReport.h"
#import "FIRCLSLogger.h"

NSString *const FIRCLSCacheDirectoryName = @"com.crashlytics.data";
NSString *const FIRCLSCacheVersion = @"v5";

@interface FIRCLSFileManager () {
  NSString *_rootPath;
}

@end

@implementation FIRCLSFileManager

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _underlyingFileManager = [NSFileManager defaultManager];

  NSString *path =
      [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) lastObject];
  path = [path stringByAppendingPathComponent:FIRCLSCacheDirectoryName];
  path = [path stringByAppendingPathComponent:[self pathNamespace]];
  _rootPath = [path copy];

  return self;
}

#pragma mark - Core API

- (BOOL)fileExistsAtPath:(NSString *)path {
  return [_underlyingFileManager fileExistsAtPath:path];
}

- (BOOL)createFileAtPath:(NSString *)path
                contents:(nullable NSData *)data
              attributes:(nullable NSDictionary<NSFileAttributeKey, id> *)attr {
  return [_underlyingFileManager createFileAtPath:path contents:data attributes:attr];
}

- (BOOL)createDirectoryAtPath:(NSString *)path {
  NSDictionary *attributes;
  NSError *error;

  attributes = @{NSFilePosixPermissions : [NSNumber numberWithShort:0755]};
  error = nil;

  if (![[self underlyingFileManager] createDirectoryAtPath:path
                               withIntermediateDirectories:YES
                                                attributes:attributes
                                                     error:&error]) {
    FIRCLSErrorLog(@"Unable to create directory %@", error);
    return NO;
  }

  return YES;
}

- (BOOL)removeItemAtPath:(NSString *)path {
  NSError *error;

  error = nil;
  if (![[self underlyingFileManager] removeItemAtPath:path error:&error] || !path) {
    FIRCLSErrorLog(@"Failed to remove file %@: %@", path, error);

    return NO;
  }

  return YES;
}

- (BOOL)removeContentsOfDirectoryAtPath:(NSString *)path {
  __block BOOL success = YES;

  // only return true if we were able to remove every item in the directory (or it was empty)

  [self enumerateFilesInDirectory:path
                       usingBlock:^(NSString *filePath, NSString *extension) {
                         success = [self removeItemAtPath:filePath] && success;
                       }];

  return success;
}

- (BOOL)moveItemAtPath:(NSString *)path toDirectory:(NSString *)destDir {
  NSString *destPath;
  NSError *error;

  destPath = [destDir stringByAppendingPathComponent:[path lastPathComponent]];
  error = nil;

  if (!path || !destPath) {
    FIRCLSErrorLog(@"Failed to move file, inputs invalid");

    return NO;
  }

  if (![[self underlyingFileManager] moveItemAtPath:path toPath:destPath error:&error]) {
    FIRCLSErrorLog(@"Failed to move file: %@", error);

    return NO;
  }

  return YES;
}

- (void)enumerateFilesInDirectory:(NSString *)directory
                       usingBlock:(void (^)(NSString *filePath, NSString *extension))block {
  for (NSString *path in [[self underlyingFileManager] contentsOfDirectoryAtPath:directory
                                                                           error:nil]) {
    NSString *extension;
    NSString *fullPath;

    // Skip files that start with a dot.  This is important, because if you try to move a .DS_Store
    // file, it will fail if the target directory also has a .DS_Store file in it.  Plus, its
    // wasteful, because we don't care about dot files.
    if ([path hasPrefix:@"."]) {
      continue;
    }

    extension = [path pathExtension];
    fullPath = [directory stringByAppendingPathComponent:path];
    if (block) {
      block(fullPath, extension);
    }
  }
}

- (NSNumber *)fileSizeAtPath:(NSString *)path {
  NSError *error = nil;
  NSDictionary *attrs = [[self underlyingFileManager] attributesOfItemAtPath:path error:&error];

  if (!attrs) {
    FIRCLSErrorLog(@"Unable to read file size: %@", error);
    return nil;
  }

  return [attrs objectForKey:NSFileSize];
}

- (NSArray *)contentsOfDirectory:(NSString *)path {
  NSMutableArray *array = [NSMutableArray array];

  [self enumerateFilesInDirectory:path
                       usingBlock:^(NSString *filePath, NSString *extension) {
                         [array addObject:filePath];
                       }];

  return [array copy];
}

#pragma - Properties
- (NSString *)pathNamespace {
  return FIRCLSApplicationGetBundleIdentifier();
}

- (NSString *)versionedPath {
  return [[self rootPath] stringByAppendingPathComponent:FIRCLSCacheVersion];
}

#pragma - Settings Paths

// This path should be different than the structurePath because the
// settings download operations will delete the settings directory,
// which would delete crash reports if these were the same
- (NSString *)settingsDirectoryPath {
  return [[self versionedPath] stringByAppendingPathComponent:@"settings"];
}

- (NSString *)settingsFilePath {
  return [[self settingsDirectoryPath] stringByAppendingPathComponent:@"settings.json"];
}

- (NSString *)settingsCacheKeyPath {
  return [[self settingsDirectoryPath] stringByAppendingPathComponent:@"cache-key.json"];
}

#pragma - Report Paths
- (NSString *)structurePath {
  return [[self versionedPath] stringByAppendingPathComponent:@"reports"];
}

- (NSString *)activePath {
  return [[self structurePath] stringByAppendingPathComponent:@"active"];
}

- (NSString *)pendingPath {
  return [[self structurePath] stringByAppendingPathComponent:@"pending"];
}

- (NSString *)processingPath {
  return [[self structurePath] stringByAppendingPathComponent:@"processing"];
}

- (NSString *)legacyPreparedPath {
  return [[self structurePath] stringByAppendingPathComponent:@"prepared-legacy"];
}

- (NSString *)preparedPath {
  return [[self structurePath] stringByAppendingPathComponent:@"prepared"];
}

- (NSArray *)activePathContents {
  return [self contentsOfDirectory:[self activePath]];
}

- (NSArray *)legacyPreparedPathContents {
  return [self contentsOfDirectory:[self legacyPreparedPath]];
}

- (NSArray *)preparedPathContents {
  return [self contentsOfDirectory:[self preparedPath]];
}

- (NSArray *)processingPathContents {
  return [self contentsOfDirectory:[self processingPath]];
}

#pragma mark - Logic
- (BOOL)createReportDirectories {
  if (![self createDirectoryAtPath:[self activePath]]) {
    return NO;
  }

  if (![self createDirectoryAtPath:[self processingPath]]) {
    return NO;
  }

  if (![self createDirectoryAtPath:[self legacyPreparedPath]]) {
    return NO;
  }

  if (![self createDirectoryAtPath:[self preparedPath]]) {
    return NO;
  }

  return YES;
}

- (NSString *)setupNewPathForExecutionIdentifier:(NSString *)identifier {
  NSString *path = [[self activePath] stringByAppendingPathComponent:identifier];

  if (![self createDirectoryAtPath:path]) {
    return nil;
  }

  return path;
}

- (BOOL)moveItemAtPath:(NSString *)srcPath toPath:(NSString *)dstPath error:(NSError **)error {
  return [self.underlyingFileManager moveItemAtPath:srcPath toPath:dstPath error:error];
}

// Wrapper over NSData so the method can be mocked for unit tests
- (NSData *)dataWithContentsOfFile:(NSString *)path {
  return [NSData dataWithContentsOfFile:path];
}

@end
