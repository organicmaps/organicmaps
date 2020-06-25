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

#import "FIRCLSdSYM.h"

#import "FIRCLSMachOBinary.h"

#define CLS_XCODE_DSYM_PREFIX (@"com.apple.xcode.dsym.")

@interface FIRCLSdSYM ()

@property(nonatomic, readonly) NSBundle* bundle;

@end

@implementation FIRCLSdSYM

+ (id)dSYMWithURL:(NSURL*)url {
  return [[self alloc] initWithURL:url];
}

- (id)initWithURL:(NSURL*)url {
  self = [super init];
  if (self) {
    NSDirectoryEnumerator* enumerator;
    NSString* path;
    NSFileManager* fileManager;
    BOOL isDirectory;
    BOOL fileExistsAtPath;
    NSArray* itemsInDWARFDir;

    fileManager = [NSFileManager defaultManager];

    // Is there a file at this path?
    if (![fileManager fileExistsAtPath:[url path]]) {
      return nil;
    }

    _bundle = [NSBundle bundleWithURL:url];
    if (!_bundle) {
      return nil;
    }

    path = [[url path] stringByAppendingPathComponent:@"Contents/Resources/DWARF"];

    // Does this path exist and is it a directory?
    fileExistsAtPath = [fileManager fileExistsAtPath:path isDirectory:&isDirectory];
    if (!fileExistsAtPath || !isDirectory) {
      return nil;
    }

    enumerator = [fileManager enumeratorAtPath:path];
    itemsInDWARFDir = [enumerator allObjects];
    // Do we have a Contents/Resources/DWARF dir but no contents?
    if ([itemsInDWARFDir count] == 0) {
      return nil;
    }

    path = [path stringByAppendingPathComponent:[itemsInDWARFDir objectAtIndex:0]];

    _binary = [[FIRCLSMachOBinary alloc] initWithURL:[NSURL fileURLWithPath:path]];
  }

  return self;
}

- (NSString*)bundleIdentifier {
  NSString* identifier;

  identifier = [_bundle bundleIdentifier];
  if ([identifier hasPrefix:CLS_XCODE_DSYM_PREFIX]) {
    return [identifier substringFromIndex:[CLS_XCODE_DSYM_PREFIX length]];
  }

  return identifier;
}

- (NSURL*)executableURL {
  return [_binary URL];
}

- (NSString*)executablePath {
  return [_binary path];
}

- (NSString*)bundleVersion {
  return [[_bundle infoDictionary] objectForKey:@"CFBundleVersion"];
}

- (NSString*)shortBundleVersion {
  return [[_bundle infoDictionary] objectForKey:@"CFBundleShortVersionString"];
}

- (void)enumerateUUIDs:(void (^)(NSString* uuid, NSString* architecture))block {
  [_binary enumerateUUIDs:block];
}

@end
