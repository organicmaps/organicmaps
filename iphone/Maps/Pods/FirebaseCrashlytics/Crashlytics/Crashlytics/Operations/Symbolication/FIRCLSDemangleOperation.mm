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

#include "FIRCLSDemangleOperation.h"
#include "FIRStackFrame_Private.h"

#import <cxxabi.h>

@implementation FIRCLSDemangleOperation

+ (NSString *)demangleSymbol:(const char *)symbol {
  if (!symbol) {
    return nil;
  }

  if (strncmp(symbol, "_Z", 2) == 0) {
    return [self demangleCppSymbol:symbol];
  } else if (strncmp(symbol, "__Z", 3) == 0) {
    return [self demangleBlockInvokeCppSymbol:symbol];
  }

  return nil;
}

+ (NSString *)demangleBlockInvokeCppSymbol:(const char *)symbol {
  NSString *string = [NSString stringWithUTF8String:symbol];

  // search backwards, because this string should be at the end
  NSRange range = [string rangeOfString:@"_block_invoke" options:NSBackwardsSearch];

  if (range.location == NSNotFound) {
    return nil;
  }

  // we need at least a "_Z..." for a valid C++ symbol, so make sure of that
  if (range.location < 5) {
    return nil;
  }

  // extract the mangled C++ symbol from the string
  NSString *cppSymbol = [string substringWithRange:NSMakeRange(1, range.location - 1)];
  cppSymbol = [self demangleSymbol:[cppSymbol UTF8String]];
  if (!cppSymbol) {
    return nil;
  }

  // extract out just the "_block_invoke..." part
  string =
      [string substringWithRange:NSMakeRange(range.location, [string length] - range.location)];

  // and glue that onto the end
  return [cppSymbol stringByAppendingString:string];
}

+ (NSString *)demangleCppSymbol:(const char *)symbol {
  int status;
  char *buffer = NULL;

  buffer = __cxxabiv1::__cxa_demangle(symbol, buffer, NULL, &status);
  if (!buffer) {
    return nil;
  }

  NSString *result = [NSString stringWithUTF8String:buffer];

  free(buffer);

  return result;
}

- (NSString *)demangleSymbol:(const char *)symbol {
  return [[self class] demangleSymbol:symbol];
}

- (void)main {
  [self enumerateFramesWithBlock:^(FIRStackFrame *frame) {
    NSString *demangedSymbol = [self demangleSymbol:[[frame rawSymbol] UTF8String]];

    if (demangedSymbol) {
      [frame setSymbol:demangedSymbol];
    }
  }];
}

@end
