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

#import "FIRCLSURLBuilder.h"

#import "FIRCLSLogger.h"

@interface FIRCLSURLBuilder ()

@property(nonatomic) NSMutableString *URLString;
@property(nonatomic) NSUInteger queryParams;

- (NSString *)escapeString:(NSString *)string;

@end

@implementation FIRCLSURLBuilder

+ (instancetype)URLWithBase:(NSString *)base {
  FIRCLSURLBuilder *url = [[FIRCLSURLBuilder alloc] init];

  [url appendComponent:base];

  return url;
}

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _URLString = [[NSMutableString alloc] init];
  _queryParams = 0;

  return self;
}

- (NSString *)escapeString:(NSString *)string {
#if TARGET_OS_WATCH
  // TODO: Question - Why does watchOS use a different encoding from the other platforms and the
  // Android SDK?
  return
      [string stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                                     URLPathAllowedCharacterSet]];
#else
  return
      [string stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet
                                                                     .URLQueryAllowedCharacterSet];
#endif
}

- (void)appendComponent:(NSString *)component {
  if (component.length == 0) {
    FIRCLSErrorLog(@"URLBuilder parameter component must not be empty");
    return;
  }

  [self.URLString appendString:component];
}

- (void)escapeAndAppendComponent:(NSString *)component {
  [self appendComponent:[self escapeString:component]];
}

- (void)appendValue:(id)value forQueryParam:(NSString *)param {
  if (!value) {
    return;
  }

  if (self.queryParams == 0) {
    [self appendComponent:@"?"];
  } else {
    [self appendComponent:@"&"];
  }

  self.queryParams += 1;

  [self appendComponent:param];
  [self appendComponent:@"="];
  if ([value isKindOfClass:NSString.class]) {
    [self escapeAndAppendComponent:value];
  } else {
    [self escapeAndAppendComponent:[value description]];
  }
}

- (NSURL *)URL {
  return [NSURL URLWithString:self.URLString];
}

@end
