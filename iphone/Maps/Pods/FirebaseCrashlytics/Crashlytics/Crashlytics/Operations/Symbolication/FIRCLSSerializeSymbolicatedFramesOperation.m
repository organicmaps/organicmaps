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

#import "FIRCLSSerializeSymbolicatedFramesOperation.h"

#import "FIRCLSFile.h"
#import "FIRCLSLogger.h"
#import "FIRStackFrame_Private.h"

@implementation FIRCLSSerializeSymbolicatedFramesOperation

- (void)main {
  FIRCLSFile file;

  // Make sure not to open in append mode, so we can overwrite any pre-existing symbolication
  // files.
  if (!FIRCLSFileInitWithPathMode(&file, [self.outputPath fileSystemRepresentation], false,
                                  false)) {
    FIRCLSErrorLog(@"Failed to create output file");
    return;
  }

  FIRCLSFileWriteSectionStart(&file, "threads");
  FIRCLSFileWriteArrayStart(&file);

  for (NSArray *frameArray in self.threadArray) {
    FIRCLSFileWriteArrayStart(&file);

    for (FIRStackFrame *frame in frameArray) {
      FIRCLSFileWriteHashStart(&file);
      FIRCLSFileWriteHashEntryString(&file, "symbol", [[frame symbol] UTF8String]);

      // only include this field if it is present and different
      if (![[frame rawSymbol] isEqualToString:[frame symbol]]) {
        FIRCLSFileWriteHashEntryString(&file, "raw_symbol", [[frame rawSymbol] UTF8String]);
      }

      FIRCLSFileWriteHashEntryUint64(&file, "offset", [frame offset]);
      FIRCLSFileWriteHashEntryString(&file, "library", [[frame library] UTF8String]);

      FIRCLSFileWriteHashEnd(&file);
    }

    FIRCLSFileWriteArrayEnd(&file);
  }

  FIRCLSFileWriteArrayEnd(&file);
  FIRCLSFileWriteSectionEnd(&file);
  FIRCLSFileClose(&file);
}

@end
