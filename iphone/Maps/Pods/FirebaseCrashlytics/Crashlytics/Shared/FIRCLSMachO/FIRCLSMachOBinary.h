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
#import "FIRCLSMachO.h"

@class FIRCLSMachOSlice;

@interface FIRCLSMachOBinary : NSObject {
  NSURL* _url;

  struct FIRCLSMachOFile _file;
  NSMutableArray* _slices;
  NSString* _instanceIdentifier;
}

+ (id)MachOBinaryWithPath:(NSString*)path;

- (id)initWithURL:(NSURL*)url;

@property(nonatomic, copy, readonly) NSURL* URL;
@property(nonatomic, copy, readonly) NSString* path;
@property(nonatomic, strong, readonly) NSArray* slices;
@property(nonatomic, copy, readonly) NSString* instanceIdentifier;

- (void)enumerateUUIDs:(void (^)(NSString* uuid, NSString* architecture))block;

- (FIRCLSMachOSlice*)sliceForArchitecture:(NSString*)architecture;

@end
