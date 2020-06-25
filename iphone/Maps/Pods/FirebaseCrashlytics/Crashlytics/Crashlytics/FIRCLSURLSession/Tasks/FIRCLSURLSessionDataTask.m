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
#import "FIRCLSURLSessionDataTask.h"

#import "FIRCLSURLSessionDataTask_PrivateMethods.h"

#define DELEGATE ((id<FIRCLSURLSessionDataDelegate>)[self delegate])

@interface FIRCLSURLSessionDataTask () <NSURLConnectionDelegate>
@end

@implementation FIRCLSURLSessionDataTask

@synthesize connection = _connection;
@synthesize completionHandler = _completionHandler;
@synthesize taskDescription = _taskDescription;

#if !__has_feature(objc_arc)
- (void)dealloc {
  [_connection release];
  [_completionHandler release];
  [_taskDescription release];
  [_data release];

  [super dealloc];
}
#endif

- (void)resume {
  dispatch_async([self queue], ^{
    NSURLConnection *connection;

    if ([self connection]) {
      return;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    connection = [[NSURLConnection alloc] initWithRequest:[self originalRequest]
                                                 delegate:self
                                         startImmediately:NO];
#pragma clang diagnostic pop

    [self setConnection:connection];

    // bummer we have to do this on a runloop, but other mechanisms require iOS 5 or 10.7
    [connection scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
#if !__has_feature(objc_arc)
    [connection release];
#endif
    [connection start];
  });
}

- (void)complete {
  // call completion handler first
  if (_completionHandler) {
    // this should go to another queue
    _completionHandler(_data, [self response], [self error]);
  }

  // and then finally, call the session delegate completion
  [DELEGATE task:self didCompleteWithError:[self error]];
}

- (void)cancel {
  [self.connection cancel];
}

#pragma mark NSURLConnectionDelegate
- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
  dispatch_async([self queue], ^{
    [DELEGATE task:self didReceiveResponse:response];

    [self setResponse:response];
  });
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
  dispatch_async([self queue], ^{
    if (!self->_data) {
      self->_data = [NSMutableData new];
    }

    [self->_data appendData:data];
    [DELEGATE task:self didReceiveData:data];
  });
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
  dispatch_async([self queue], ^{
    [self setError:error];
    [self complete];
  });
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
  dispatch_async([self queue], ^{
    [self complete];
  });
}

@end

#else

INJECT_STRIP_SYMBOL(clsurlsessiondatatask)

#endif
