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

#import "FIRCLSProcessReportOperation.h"

#import "FIRCLSDemangleOperation.h"
#import "FIRCLSFile.h"
#import "FIRCLSInternalReport.h"
#import "FIRCLSSerializeSymbolicatedFramesOperation.h"
#import "FIRCLSSymbolResolver.h"
#import "FIRCLSSymbolicationOperation.h"
#import "FIRStackFrame_Private.h"

@implementation FIRCLSProcessReportOperation

- (instancetype)initWithReport:(FIRCLSInternalReport *)report
                      resolver:(FIRCLSSymbolResolver *)resolver {
  self = [super init];
  if (!self) {
    return nil;
  }

  _report = report;
  _symbolResolver = resolver;

  return self;
}

- (NSString *)binaryImagePath {
  return self.report.binaryImagePath;
}

- (NSArray *)threadArrayFromFile:(NSString *)path {
  NSArray *threads =
      FIRCLSFileReadSections([path fileSystemRepresentation], false, ^NSObject *(id obj) {
        // use this to select out the one entry that has a "threads" top-level entry
        return [obj objectForKey:@"threads"];
      });

  if ([threads count] == 0) {
    return nil;
  }

  // threads is actually an array of arrays
  threads = [threads objectAtIndex:0];
  if (!threads) {
    return nil;
  }

  NSMutableArray *threadArray = [NSMutableArray array];

  for (NSDictionary *threadDetails in threads) {
    NSMutableArray *frameArray = [NSMutableArray array];

    for (NSNumber *pc in [threadDetails objectForKey:@"stacktrace"]) {
      FIRStackFrame *frame = [FIRStackFrame stackFrameWithAddress:[pc unsignedIntegerValue]];

      [frameArray addObject:frame];
    }

    [threadArray addObject:frameArray];
  }

  return threadArray;
}

- (BOOL)symbolicateFile:(NSString *)path withResolver:(FIRCLSSymbolResolver *)resolver {
  NSArray *threadArray = [self threadArrayFromFile:path];
  if (!threadArray) {
    return NO;
  }

  FIRCLSSymbolicationOperation *symbolicationOp = [[FIRCLSSymbolicationOperation alloc] init];
  [symbolicationOp setThreadArray:threadArray];
  [symbolicationOp setSymbolResolver:resolver];

  FIRCLSDemangleOperation *demangleOp = [[FIRCLSDemangleOperation alloc] init];
  [demangleOp setThreadArray:threadArray];

  FIRCLSSerializeSymbolicatedFramesOperation *serializeOp =
      [[FIRCLSSerializeSymbolicatedFramesOperation alloc] init];
  [serializeOp setThreadArray:threadArray];
  [serializeOp setOutputPath:[path stringByAppendingPathExtension:@"symbolicated"]];

  [symbolicationOp start];
  [demangleOp start];
  [serializeOp start];

  return YES;
}

- (void)main {
  if (![self.symbolResolver loadBinaryImagesFromFile:self.binaryImagePath]) {
    return;
  }

  [self.report enumerateSymbolicatableFilesInContent:^(NSString *path) {
    [self symbolicateFile:path withResolver:self.symbolResolver];
  }];
}

@end
