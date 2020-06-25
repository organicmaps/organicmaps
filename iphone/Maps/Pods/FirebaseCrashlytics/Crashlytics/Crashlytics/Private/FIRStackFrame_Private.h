// Copyright 2020 Google
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

#import "FIRStackFrame.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * This class is used in conjunction with recordExceptionModel to record information about
 * non-ObjC/C++ exceptions. All information included here will be displayed in the Crashlytics UI,
 * and can influence crash grouping. Be particularly careful with the use of the address property.
 *If set, Crashlytics will attempt symbolication and could overwrite other properities in the
 *process.
 **/
@interface FIRStackFrame (Private)

+ (instancetype)stackFrame;
+ (instancetype)stackFrameWithAddress:(NSUInteger)address;
+ (instancetype)stackFrameWithSymbol:(NSString *)symbol;

@property(nonatomic, copy, nullable) NSString *symbol;
@property(nonatomic, copy, nullable) NSString *rawSymbol;
@property(nonatomic, copy, nullable) NSString *library;
@property(nonatomic, copy, nullable) NSString *fileName;
@property(nonatomic, assign) uint32_t lineNumber;
@property(nonatomic, assign) uint64_t offset;
@property(nonatomic, assign) uint64_t address;

@end

NS_ASSUME_NONNULL_END
