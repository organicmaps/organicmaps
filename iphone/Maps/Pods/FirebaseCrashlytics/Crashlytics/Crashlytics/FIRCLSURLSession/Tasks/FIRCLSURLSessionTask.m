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

#import "FIRCLSURLSessionAvailability.h"

#if FIRCLSURLSESSION_REQUIRED
#import "FIRCLSURLSession.h"

#import "FIRCLSURLSessionTask.h"

#import "FIRCLSURLSessionTask_PrivateMethods.h"
#import "FIRCLSURLSession_PrivateMethods.h"

@implementation FIRCLSURLSessionTask

+ (instancetype)task {
#if __has_feature(objc_arc)
  return [[self class] new];

#else
  return [[[self class] new] autorelease];
#endif
}

@synthesize currentRequest = _currentRequest;
@synthesize originalRequest = _originalRequest;
@synthesize response = _response;
@synthesize error = _error;
@synthesize queue = _queue;
@synthesize invokesDelegate = _invokesDelegate;

- (instancetype)init {
  self = [super init];
  if (!self) {
    return self;
  }

  _queue = dispatch_queue_create("com.crashlytics.URLSessionTask", 0);

  _invokesDelegate = YES;

  return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc {
  [_originalRequest release];
  [_currentRequest release];
  [_response release];
  [_error release];

#if !OS_OBJECT_USE_OBJC
  dispatch_release(_queue);
#endif

  [super dealloc];
}
#endif

- (void)start {
#if DEBUG
  assert(0 && "Must be implemented by FIRCLSURLSessionTask subclasses");
#endif
}

- (void)cancel {
#if DEBUG
  assert(0 && "Must be implemented by FIRCLSURLSessionTask subclasses");
#endif
}

- (void)resume {
}

- (void)cleanup {
}

@end

#else

INJECT_STRIP_SYMBOL(clsurlsessiontask)

#endif
