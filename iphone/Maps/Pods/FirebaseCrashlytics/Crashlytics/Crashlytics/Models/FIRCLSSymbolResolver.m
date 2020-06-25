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

#import "FIRCLSSymbolResolver.h"

#include <dlfcn.h>

#include "FIRCLSBinaryImage.h"
#include "FIRCLSFile.h"
#import "FIRCLSLogger.h"
#import "FIRStackFrame_Private.h"

@interface FIRCLSSymbolResolver () {
  NSMutableArray* _binaryImages;
}

@end

@implementation FIRCLSSymbolResolver

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _binaryImages = [NSMutableArray array];

  return self;
}

- (BOOL)loadBinaryImagesFromFile:(NSString*)path {
  if ([path length] == 0) {
    return NO;
  }

  NSArray* sections = FIRCLSFileReadSections([path fileSystemRepresentation], false, nil);

  if ([sections count] == 0) {
    FIRCLSErrorLog(@"Failed to read binary image file %@", path);
    return NO;
  }

  // filter out unloads, as well as loads with invalid entries
  for (NSDictionary* entry in sections) {
    NSDictionary* details = [entry objectForKey:@"load"];
    if (!details) {
      continue;
    }

    // This does happen occationally and causes a crash. I'm really not sure there
    // is anything sane we can do in this case.
    if (![details objectForKey:@"base"] || ![details objectForKey:@"size"]) {
      continue;
    }

    if ([details objectForKey:@"base"] == (id)[NSNull null] ||
        [details objectForKey:@"size"] == (id)[NSNull null]) {
      continue;
    }

    [_binaryImages addObject:details];
  }

  [_binaryImages sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
    NSNumber* base1 = [obj1 objectForKey:@"base"];
    NSNumber* base2 = [obj2 objectForKey:@"base"];

    return [base1 compare:base2];
  }];

  return YES;
}

- (NSDictionary*)loadedBinaryImageForPC:(uintptr_t)pc {
  NSUInteger index =
      [_binaryImages indexOfObjectPassingTest:^BOOL(id obj, NSUInteger idx, BOOL* stop) {
        uintptr_t base = [[obj objectForKey:@"base"] unsignedIntegerValue];
        uintptr_t size = [[obj objectForKey:@"size"] unsignedIntegerValue];

        return pc >= base && pc < (base + size);
      }];

  if (index == NSNotFound) {
    return nil;
  }

  return [_binaryImages objectAtIndex:index];
}

- (BOOL)fillInImageDetails:(FIRCLSBinaryImageDetails*)details forUUID:(NSString*)uuid {
  if (!details || !uuid) {
    return NO;
  }

  return FIRCLSBinaryImageFindImageForUUID([uuid UTF8String], details);
}

- (FIRStackFrame*)frameForAddress:(uint64_t)address {
  FIRStackFrame* frame = [FIRStackFrame stackFrameWithAddress:(NSUInteger)address];

  if (![self updateStackFrame:frame]) {
    return nil;
  }

  return frame;
}

- (BOOL)updateStackFrame:(FIRStackFrame*)frame {
  uint64_t address = [frame address];
  if (address == 0) {
    return NO;
  }

  NSDictionary* binaryImage = [self loadedBinaryImageForPC:(uintptr_t)address];

  FIRCLSBinaryImageDetails imageDetails;

  if (![self fillInImageDetails:&imageDetails forUUID:[binaryImage objectForKey:@"uuid"]]) {
#if DEBUG
    FIRCLSDebugLog(@"Image not found");
#endif
    return NO;
  }

  uintptr_t addr = (uintptr_t)address -
                   (uintptr_t)[[binaryImage objectForKey:@"base"] unsignedIntegerValue] +
                   (uintptr_t)imageDetails.node.baseAddress;
  Dl_info dlInfo;

  if (dladdr((void*)addr, &dlInfo) == 0) {
#if DEBUG
    FIRCLSDebugLog(@"Could not look up address");
#endif
    return NO;
  }

  if (addr - (uintptr_t)dlInfo.dli_saddr == 0) {
    addr -= 2;
    if (dladdr((void*)addr, &dlInfo) == 0) {
#if DEBUG
      FIRCLSDebugLog(@"Could not look up address");
#endif
      return NO;
    }
  }

  if (dlInfo.dli_sname) {
    NSString* symbol = [NSString stringWithUTF8String:dlInfo.dli_sname];

    frame.symbol = symbol;
    frame.rawSymbol = symbol;
  }

  if (addr > (uintptr_t)dlInfo.dli_saddr) {
    [frame setOffset:addr - (uintptr_t)dlInfo.dli_saddr];
  }

  [frame setLibrary:[[binaryImage objectForKey:@"path"] lastPathComponent]];

  return YES;
}

@end
