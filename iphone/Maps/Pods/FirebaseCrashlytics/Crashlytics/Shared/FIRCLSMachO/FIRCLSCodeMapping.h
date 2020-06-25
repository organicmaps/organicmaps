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

typedef enum {
  FIRCLSCodeMappingSourceUnknown,
  FIRCLSCodeMappingSourceBuild,
  FIRCLSCodeSourceCache,
  FIRCLSCodeSourceSpotlight
} FIRCLSCodeMappingSource;

@interface FIRCLSCodeMapping : NSObject

+ (instancetype)mappingWithURL:(NSURL*)URL;

- (instancetype)initWithURL:(NSURL*)URL;

@property(nonatomic, copy, readonly) NSURL* URL;
@property(nonatomic, assign) FIRCLSCodeMappingSource source;
@property(nonatomic, copy, readonly) NSString* sourceName;

@end
