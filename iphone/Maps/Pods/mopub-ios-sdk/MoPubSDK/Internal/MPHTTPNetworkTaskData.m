//
//  MPHTTPNetworkTaskData.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPHTTPNetworkTaskData.h"

NS_ASSUME_NONNULL_BEGIN
@implementation MPHTTPNetworkTaskData

- (instancetype)init {
    return [self initWithResponseHandler:nil errorHandler:nil shouldRedirectWithNewRequest:nil];
}

- (instancetype)initWithResponseHandler:(void (^ _Nullable)(NSData * data, NSHTTPURLResponse * response))responseHandler
                           errorHandler:(void (^ _Nullable)(NSError * error))errorHandler
           shouldRedirectWithNewRequest:(BOOL (^ _Nullable)(NSURLSessionTask * task, NSURLRequest * newRequest))shouldRedirectWithNewRequest {
    if (self = [super init]) {
        _responseData = nil;
        _responseHandler = responseHandler;
        _errorHandler = errorHandler;
        _shouldRedirectWithNewRequest = shouldRedirectWithNewRequest;
    }

    return self;
}

@end
NS_ASSUME_NONNULL_END
