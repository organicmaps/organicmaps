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

@class FIRCLSMachOBinary;

@interface FIRCLSdSYM : NSObject

NS_ASSUME_NONNULL_BEGIN

+ (id)dSYMWithURL:(NSURL*)url;

- (id)initWithURL:(NSURL*)url;

@property(nonatomic, readonly) FIRCLSMachOBinary* binary;
@property(nonatomic, copy, readonly, nullable) NSString* bundleIdentifier;
@property(nonatomic, copy, readonly) NSURL* executableURL;
@property(nonatomic, copy, readonly) NSString* executablePath;
@property(nonatomic, copy, readonly) NSString* bundleVersion;
@property(nonatomic, copy, readonly) NSString* shortBundleVersion;

- (void)enumerateUUIDs:(void (^)(NSString* uuid, NSString* architecture))block;

NS_ASSUME_NONNULL_END

@end
