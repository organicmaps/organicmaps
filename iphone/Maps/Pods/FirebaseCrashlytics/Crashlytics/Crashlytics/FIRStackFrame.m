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

#import "FIRStackFrame_Private.h"

@interface FIRStackFrame ()

@property(nonatomic, copy, nullable) NSString *symbol;
@property(nonatomic, copy, nullable) NSString *rawSymbol;
@property(nonatomic, copy, nullable) NSString *library;
@property(nonatomic, copy, nullable) NSString *fileName;
@property(nonatomic, assign) uint32_t lineNumber;
@property(nonatomic, assign) uint64_t offset;
@property(nonatomic, assign) uint64_t address;

@property(nonatomic, assign) BOOL isSymbolicated;

@end

@implementation FIRStackFrame

#pragma mark - Public Methods

- (instancetype)initWithSymbol:(NSString *)symbol file:(NSString *)file line:(NSInteger)line {
  self = [super init];
  if (!self) {
    return nil;
  }

  _symbol = [symbol copy];
  _fileName = [file copy];
  _lineNumber = (uint32_t)line;

  _isSymbolicated = true;

  return self;
}

+ (instancetype)stackFrameWithSymbol:(NSString *)symbol file:(NSString *)file line:(NSInteger)line {
  return [[FIRStackFrame alloc] initWithSymbol:symbol file:file line:line];
}

#pragma mark - Internal Methods

+ (instancetype)stackFrame {
  return [[self alloc] init];
}

+ (instancetype)stackFrameWithAddress:(NSUInteger)address {
  FIRStackFrame *frame = [self stackFrame];

  [frame setAddress:address];

  return frame;
}

+ (instancetype)stackFrameWithSymbol:(NSString *)symbol {
  FIRStackFrame *frame = [self stackFrame];

  frame.symbol = symbol;
  frame.rawSymbol = symbol;

  return frame;
}

#pragma mark - Overrides

- (NSString *)description {
  if (self.isSymbolicated) {
    return [NSString
        stringWithFormat:@"{%@ - %@:%u}", [self fileName], [self symbol], [self lineNumber]];
  }

  if (self.fileName) {
    return [NSString stringWithFormat:@"{[0x%llx] %@ - %@:%u}", [self address], [self fileName],
                                      [self symbol], [self lineNumber]];
  }

  return [NSString
      stringWithFormat:@"{[0x%llx + %u] %@}", [self address], [self lineNumber], [self symbol]];
}

@end
