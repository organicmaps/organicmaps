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

@protocol FIRCLSURLSessionTaskDelegate;

@interface FIRCLSURLSessionTask : NSObject {
  __unsafe_unretained id<FIRCLSURLSessionTaskDelegate> _delegate;

  NSURLRequest* _originalRequest;
  NSURLRequest* _currentRequest;
  NSURLResponse* _response;
  NSError* _error;
  dispatch_queue_t _queue;
  BOOL _invokesDelegate;
}

@property(nonatomic, readonly, copy) NSURLRequest* originalRequest;
@property(nonatomic, readonly, copy) NSURLRequest* currentRequest;
@property(nonatomic, readonly, copy) NSURLResponse* response;

@property(nonatomic, readonly, copy) NSError* error;

- (void)resume;

@end
