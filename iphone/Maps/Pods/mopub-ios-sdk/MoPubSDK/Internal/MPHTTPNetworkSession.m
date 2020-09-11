//
//  MPHTTPNetworkSession.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPError.h"
#import "MPHTTPNetworkTaskData.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "NSError+MPAdditions.h"

// Macros for dispatching asynchronously to the main queue
#define safe_block(block, ...) block ? block(__VA_ARGS__) : nil
#define async_queue_block(queue, block, ...) dispatch_async(queue, ^ \
{ \
safe_block(block, __VA_ARGS__); \
})
#define main_queue_block(block, ...) async_queue_block(dispatch_get_main_queue(), block, __VA_ARGS__);

// Constants
NSString * const kMoPubSDKNetworkDomain = @"MoPubSDKNetworkDomain";

@interface MPHTTPNetworkSession() <NSURLSessionDataDelegate>
@property (nonatomic, strong) NSURLSession * sharedSession;

// Access to `NSMutableDictionary` is not thread-safe by default, so we will gate access
// to it using GCD to allow concurrent reads, but synchronous writes.
@property (nonatomic, strong) NSMutableDictionary<NSURLSessionTask *, MPHTTPNetworkTaskData *> * sessions;
@property (nonatomic, strong) dispatch_queue_t sessionsQueue;
@end

@implementation MPHTTPNetworkSession

#pragma mark - Initialization

+ (instancetype)sharedInstance {
    static dispatch_once_t once;
    static id _sharedInstance;
    dispatch_once(&once, ^{
        _sharedInstance = [[self alloc] init];
    });
    return _sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        // Shared `NSURLSession` to be used for all `MPHTTPNetworkTask` objects. All tasks should use this single
        // session so that the DNS lookup and SSL handshakes do not need to be redone.
        _sharedSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration] delegate:self delegateQueue:nil];

        // Dictionary of all sessions currently in flight.
        _sessions = [NSMutableDictionary dictionary];
        _sessionsQueue = dispatch_queue_create("com.mopub.mopub-ios-sdk.mphttpnetworksession.queue", DISPATCH_QUEUE_CONCURRENT);
    }

    return self;
}

#pragma mark - Session Access

- (void)setSessionData:(MPHTTPNetworkTaskData *)data forTask:(NSURLSessionTask *)task {
    dispatch_barrier_sync(self.sessionsQueue, ^{
        self.sessions[task] = data;
    });
}

/**
 Retrieves the task data for the specified task. Accessing the data is thread
 safe, but mutating the data is not thread safe.
 @param task Task which needs a data retrieval.
 @return The task data or @c nil
 */
- (MPHTTPNetworkTaskData *)sessionDataForTask:(NSURLSessionTask *)task {
    __block MPHTTPNetworkTaskData * data = nil;
    dispatch_sync(self.sessionsQueue, ^{
        data = self.sessions[task];
    });

    return data;
}

/**
 Appends additional data to the @c responseData field of @c MPHTTPNetworkTaskData in
 a thread safe manner.
 @param data New data to append.
 @param task Task to append the data to.
 */
- (void)appendData:(NSData *)data toSessionDataForTask:(NSURLSessionTask *)task {
    // No data to append or task.
    if (data == nil || task == nil) {
        return;
    }

    dispatch_barrier_sync(self.sessionsQueue, ^{
        // Do nothing if there is no task data entry.
        MPHTTPNetworkTaskData * taskData = self.sessions[task];
        if (taskData == nil) {
            return;
        }

        // Append the new data to the task.
        if (taskData.responseData == nil) {
            taskData.responseData = [NSMutableData data];
        }

        [taskData.responseData appendData:data];
    });
}

#pragma mark - Manual Start Tasks

+ (NSURLSessionTask *)taskWithHttpRequest:(NSURLRequest *)request
                          responseHandler:(void (^ _Nullable)(NSData * data, NSHTTPURLResponse * response))responseHandler
                             errorHandler:(void (^ _Nullable)(NSError * error))errorHandler
             shouldRedirectWithNewRequest:(BOOL (^ _Nullable)(NSURLSessionTask * task, NSURLRequest * newRequest))shouldRedirectWithNewRequest {
    // Networking task
    NSURLSessionDataTask * task = [MPHTTPNetworkSession.sharedInstance.sharedSession dataTaskWithRequest:request];

    // Initialize the task data
    MPHTTPNetworkTaskData * taskData = [[MPHTTPNetworkTaskData alloc] initWithResponseHandler:responseHandler errorHandler:errorHandler shouldRedirectWithNewRequest:shouldRedirectWithNewRequest];

    // Update the sessions.
    [MPHTTPNetworkSession.sharedInstance setSessionData:taskData forTask:task];

    return task;
}

#pragma mark - Automatic Start Tasks

+ (NSURLSessionTask *)startTaskWithHttpRequest:(NSURLRequest *)request {
    return [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:nil errorHandler:nil shouldRedirectWithNewRequest:nil];
}

+ (NSURLSessionTask *)startTaskWithHttpRequest:(NSURLRequest *)request
                               responseHandler:(void (^ _Nullable)(NSData * data, NSHTTPURLResponse * response))responseHandler
                                  errorHandler:(void (^ _Nullable)(NSError * error))errorHandler {
    return [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:responseHandler errorHandler:errorHandler shouldRedirectWithNewRequest:nil];
}

+ (NSURLSessionTask *)startTaskWithHttpRequest:(NSURLRequest *)request
                               responseHandler:(void (^ _Nullable)(NSData * data, NSHTTPURLResponse * response))responseHandler
                                  errorHandler:(void (^ _Nullable)(NSError * error))errorHandler
                  shouldRedirectWithNewRequest:(BOOL (^ _Nullable)(NSURLSessionTask * task, NSURLRequest * newRequest))shouldRedirectWithNewRequest {
    // Generate a manual start task.
    NSURLSessionTask * task = [MPHTTPNetworkSession taskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
        main_queue_block(responseHandler, data, response);
    } errorHandler:^(NSError * _Nonnull error) {
        main_queue_block(errorHandler, error);
    } shouldRedirectWithNewRequest:shouldRedirectWithNewRequest];

    // Immediately start the task.
    [task resume];

    return task;
}

#pragma mark - NSURLSessionDataDelegate

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
    // Allow all responses.
    completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {

    // Append the new data to the task.
    [self appendData:data toSessionDataForTask:dataTask];
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
        newRequest:(NSURLRequest *)request
 completionHandler:(void (^)(NSURLRequest * _Nullable))completionHandler {
    // Retrieve the task data.
    MPHTTPNetworkTaskData * taskData = [self sessionDataForTask:task];
    if (taskData == nil) {
        completionHandler(request);
        return;
    }

    // If there is a redirection handler block registered with the HTTP task, we should
    // query for it's response. By default, we will allow the redirection.
    NSURLRequest * newRequest = request;
    if (taskData.shouldRedirectWithNewRequest != nil && !taskData.shouldRedirectWithNewRequest(task, request)) {
        // Reject the redirection.
        newRequest = nil;
    }

    completionHandler(newRequest);
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
didCompleteWithError:(nullable NSError *)error {
    // Retrieve the task data.
    MPHTTPNetworkTaskData * taskData = [self sessionDataForTask:task];
    if (taskData == nil) {
        return;
    }

    // Remove the task data from the currently in flight sessions.
    [self setSessionData:nil forTask:task];

    // Validate that response is not an error.
    if (error != nil) {
        MPLogEvent([MPLogEvent error:error message:nil]);
        safe_block(taskData.errorHandler, error);
        return;
    }

    // Validate response is a HTTP response.
    NSHTTPURLResponse * httpResponse = [task.response isKindOfClass:[NSHTTPURLResponse class]] ? (NSHTTPURLResponse *)task.response : nil;
    if (httpResponse == nil) {
        NSError * notHttpResponseError = [NSError networkResponseIsNotHTTP];
        MPLogEvent([MPLogEvent error:notHttpResponseError message:nil]);
        safe_block(taskData.errorHandler, notHttpResponseError);
        return;
    }

    // Validate response code is not an error (>= 400)
    // See https://en.wikipedia.org/wiki/List_of_HTTP_status_codes for all valid status codes.
    if (httpResponse.statusCode >= 400) {
        NSError * not200ResponseError = [NSError networkErrorWithHTTPStatusCode:httpResponse.statusCode];
        MPLogEvent([MPLogEvent error:not200ResponseError message:nil]);
        safe_block(taskData.errorHandler, not200ResponseError);
        return;
    }

    // Validate that there is data
    if (taskData.responseData == nil) {
        NSError * noDataError = [NSError networkResponseContainedNoData];
        MPLogEvent([MPLogEvent error:noDataError message:nil]);
        safe_block(taskData.errorHandler, noDataError);
        return;
    }

    // By this point all of the fields have been validated.
    safe_block(taskData.responseHandler, taskData.responseData, httpResponse);
}

@end
