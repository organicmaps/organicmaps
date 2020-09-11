//
//  MPHTTPNetworkTaskData.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

/**
 Provides an easy encapsulation of a HTTP networking task's data because we can't effectively
 subclass @c NSURLSessionDataTask.
 */
NS_ASSUME_NONNULL_BEGIN
@interface MPHTTPNetworkTaskData : NSObject
@property (nonatomic, strong, nullable) NSMutableData * responseData;
@property (nonatomic, copy, nullable) void (^responseHandler)(NSData * data, NSHTTPURLResponse * response);
@property (nonatomic, copy, nullable) void (^errorHandler)(NSError * error);
@property (nonatomic, copy, nullable) BOOL (^shouldRedirectWithNewRequest)(NSURLSessionTask * task, NSURLRequest * newRequest);

/**
 Initializes the task data with the given handlers.
 @param responseHandler Optional response handler that will be invoked on the current thread.
 @param errorHandler Optional error handler that will be invoked on the current thread.
 @param shouldRedirectWithNewRequest Optional logic control block to determine if a redirection should occur. This is invoked on the current thread.
 @returns The HTTP networking task data.
 */
- (instancetype)initWithResponseHandler:(void (^ _Nullable)(NSData * data, NSHTTPURLResponse * response))responseHandler
                           errorHandler:(void (^ _Nullable)(NSError * error))errorHandler
           shouldRedirectWithNewRequest:(BOOL (^ _Nullable)(NSURLSessionTask * task, NSURLRequest * newRequest))shouldRedirectWithNewRequest NS_DESIGNATED_INITIALIZER;

@end
NS_ASSUME_NONNULL_END
