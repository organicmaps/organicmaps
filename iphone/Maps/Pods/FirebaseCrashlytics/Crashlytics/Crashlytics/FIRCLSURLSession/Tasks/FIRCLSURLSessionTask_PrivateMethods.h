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

#import <Foundation/Foundation.h>

@protocol FIRCLSURLSessionTaskDelegate;

@interface FIRCLSURLSessionTask ()

+ (instancetype)task;

@property(nonatomic, assign) id<FIRCLSURLSessionTaskDelegate> delegate;

@property(nonatomic, copy) NSURLRequest *originalRequest;
@property(nonatomic, copy) NSURLRequest *currentRequest;
@property(nonatomic, copy) NSURLResponse *response;

@property(nonatomic, readonly) dispatch_queue_t queue;
@property(nonatomic, assign) BOOL invokesDelegate;

- (void)cancel;

@property(nonatomic, copy) NSError *error;

- (void)cleanup;

@end

@protocol FIRCLSURLSessionTaskDelegate <NSObject>
@required

- (NSURLRequest *)task:(FIRCLSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response
                    newRequest:(NSURLRequest *)request;

- (void)task:(FIRCLSURLSessionTask *)task didCompleteWithError:(NSError *)error;

@end

#endif
