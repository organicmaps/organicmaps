//
//  MPRetryingHTTPOperation.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRetryingHTTPOperation.h"

#import "MPLogging.h"

NSString * const MPRetryingHTTPOperationErrorDomain = @"com.mopub.MPRetryingHTTPOperation";
static const NSUInteger kMaximumFailedRetryAttempts = 5;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPRetryingHTTPOperation () <NSURLConnectionDataDelegate>

@property (copy, readwrite) NSURLRequest *request;
@property (strong) NSURLConnection *connection;
@property (copy, readwrite) NSHTTPURLResponse *lastResponse;
@property (strong, readwrite) NSMutableData *lastReceivedData;
@property (assign) NSUInteger failedRetryAttempts;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPRetryingHTTPOperation

- (instancetype)initWithRequest:(NSURLRequest *)request
{
    NSAssert(request != nil, @"-initWithRequest: cannot take a nil request.");
    NSAssert([request URL] != nil, @"-initWithRequest: cannot take a request whose URL is nil.");
    
    NSString *scheme = [[[request URL] scheme] lowercaseString];
    NSAssert([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"], @"-initWithRequest: can only take a request whose URL has an HTTP/HTTPS scheme.");
    
    self = [super init];
    if (self) {
        _request = [request copy];
        _connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
    }
    return self;
}

#pragma mark - MPQRunLoopOperation overrides

- (void)operationDidStart
{
    [super operationDidStart];
    
    MPLogDebug(@"Starting request: %@.", self.request);
    [self.connection scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.connection start];
}

- (void)operationWillFinish
{
    [super operationWillFinish];
    
    [self.connection cancel];
    self.connection = nil;
}

#pragma mark - Internal

- (BOOL)shouldRetryForResponse:(NSHTTPURLResponse *)response
{
    return response.statusCode == 503 || response.statusCode == 504;
}

- (NSTimeInterval)retryDelayForFailedAttempts:(NSUInteger)failedAttempts
{
    if (failedAttempts == 0) {
        // Return a short delay if this method is called when there have been no failed retries.
        return 1;
    } else {
        return pow(2, failedAttempts - 1) * 60;
    }
}

- (void)retry
{
    NSAssert([self isActualRunLoopThread], @"Retries should occur on the run loop thread.");
    
    MPLogDebug(@"Retrying request: %@.", self.request);
    
    [self.lastReceivedData setLength:0];
    
    self.connection = [[NSURLConnection alloc] initWithRequest:self.request delegate:self startImmediately:NO];
    [self.connection scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.connection start];
}

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    NSAssert([self isActualRunLoopThread], @"NSURLConnection callbacks should occur on the run loop thread.");
    NSAssert([response isKindOfClass:[NSHTTPURLResponse class]], @"Response must be of type NSHTTPURLResponse.");
    
    self.lastResponse = (NSHTTPURLResponse *)response;
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    NSAssert([self isActualRunLoopThread], @"NSURLConnection callbacks should occur on the run loop thread.");
    
    if (!self.lastReceivedData) {
        self.lastReceivedData = [NSMutableData data];
    }
    
    [self.lastReceivedData appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    NSAssert([self isActualRunLoopThread], @"NSURLConnection callbacks should occur on the run loop thread.");
    
    if (self.lastResponse.statusCode == 200) {
        MPLogDebug(@"Successful request: %@.", self.request);
        [self finishWithError:nil];
    } else if (self.failedRetryAttempts > kMaximumFailedRetryAttempts) {
        MPLogDebug(@"Too many failed attempts for this request: %@.", self.request);
        [self finishWithError:[NSError errorWithDomain:MPRetryingHTTPOperationErrorDomain code:MPRetryingHTTPOperationExceededRetryLimit userInfo:nil]];
    } else if ([self shouldRetryForResponse:self.lastResponse]) {
        self.failedRetryAttempts++;
        NSTimeInterval retryDelay = [self retryDelayForFailedAttempts:self.failedRetryAttempts];
        MPLogDebug(@"Server error during attempt #%@ for request: %@.", @(self.failedRetryAttempts), self.request);
        MPLogDebug(@"Backing off: %.1f", retryDelay);
        [self performSelector:@selector(retry) withObject:nil afterDelay:retryDelay];
    } else {
        MPLogDebug(@"%@", [[NSString alloc] initWithData:self.request.HTTPBody encoding:NSUTF8StringEncoding]);
        MPLogDebug(@"Failed request: %@, status code: %ld, error: %@.", self.request, self.lastResponse.statusCode, self.error);
        [self finishWithError:[NSError errorWithDomain:MPRetryingHTTPOperationErrorDomain code:MPRetryingHTTPOperationReceivedNonRetryResponse userInfo:nil]];
    }
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    NSAssert([self isActualRunLoopThread], @"NSURLConnection callbacks should occur on the run loop thread.");
    
    [self finishWithError:error];
}

@end
