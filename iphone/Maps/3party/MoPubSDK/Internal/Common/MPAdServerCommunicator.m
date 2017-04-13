//
//  MPAdServerCommunicator.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPAdServerCommunicator.h"

#import "MPAdConfiguration.h"
#import "MPLogging.h"
#import "MPCoreInstanceProvider.h"
#import "MPLogEvent.h"
#import "MPLogEventRecorder.h"

const NSTimeInterval kRequestTimeoutInterval = 10.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerCommunicator ()

@property (nonatomic, assign, readwrite) BOOL loading;
@property (nonatomic, copy) NSURL *URL;
@property (nonatomic, strong) NSURLConnection *connection;
@property (nonatomic, strong) NSMutableData *responseData;
@property (nonatomic, strong) NSDictionary *responseHeaders;
@property (nonatomic, strong) MPLogEvent *adRequestLatencyEvent;

- (NSError *)errorForStatusCode:(NSInteger)statusCode;
- (NSURLRequest *)adRequestForURL:(NSURL *)URL;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdServerCommunicator

@synthesize delegate = _delegate;
@synthesize URL = _URL;
@synthesize connection = _connection;
@synthesize responseData = _responseData;
@synthesize responseHeaders = _responseHeaders;
@synthesize loading = _loading;

- (id)initWithDelegate:(id<MPAdServerCommunicatorDelegate>)delegate
{
    self = [super init];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (void)dealloc
{
    [self.connection cancel];

}

#pragma mark - Public

- (void)loadURL:(NSURL *)URL
{
    [self cancel];
    self.URL = URL;

    // Start tracking how long it takes to successfully or unsuccessfully retrieve an ad.
    self.adRequestLatencyEvent = [[MPLogEvent alloc] initWithEventCategory:MPLogEventCategoryRequests eventName:MPLogEventNameAdRequest];
    self.adRequestLatencyEvent.requestURI = URL.absoluteString;

    self.connection = [NSURLConnection connectionWithRequest:[self adRequestForURL:URL]
                                                    delegate:self];
    self.loading = YES;
}

- (void)cancel
{
    self.adRequestLatencyEvent = nil;
    self.loading = NO;
    [self.connection cancel];
    self.connection = nil;
    self.responseData = nil;
    self.responseHeaders = nil;
}

#pragma mark - NSURLConnection delegate (NSURLConnectionDataDelegate in iOS 5.0+)

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    if ([response respondsToSelector:@selector(statusCode)]) {
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode >= 400) {
            // Do not record a logging event if we failed to make a connection.
            self.adRequestLatencyEvent = nil;

            [connection cancel];
            self.loading = NO;
            [self.delegate communicatorDidFailWithError:[self errorForStatusCode:statusCode]];
            return;
        }
    }

    self.responseData = [NSMutableData data];
    self.responseHeaders = [(NSHTTPURLResponse *)response allHeaderFields];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.responseData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    // Do not record a logging event if we failed to make a connection.
    self.adRequestLatencyEvent = nil;

    self.loading = NO;
    [self.delegate communicatorDidFailWithError:error];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    [self.adRequestLatencyEvent recordEndTime];
    self.adRequestLatencyEvent.requestStatusCode = 200;

    MPAdConfiguration *configuration = [[MPAdConfiguration alloc]
                                         initWithHeaders:self.responseHeaders
                                         data:self.responseData];
    MPAdConfigurationLogEventProperties *logEventProperties =
        [[MPAdConfigurationLogEventProperties alloc] initWithConfiguration:configuration];

    // Do not record ads that are warming up.
    if (configuration.adUnitWarmingUp) {
        self.adRequestLatencyEvent = nil;
    } else {
        [self.adRequestLatencyEvent setLogEventProperties:logEventProperties];
        MPAddLogEvent(self.adRequestLatencyEvent);
    }

    self.loading = NO;
    [self.delegate communicatorDidReceiveAdConfiguration:configuration];
}

#pragma mark - Internal

- (NSError *)errorForStatusCode:(NSInteger)statusCode
{
    NSString *errorMessage = [NSString stringWithFormat:
                              NSLocalizedString(@"MoPub returned status code %d.",
                                                @"Status code error"),
                              statusCode];
    NSDictionary *errorInfo = [NSDictionary dictionaryWithObject:errorMessage
                                                          forKey:NSLocalizedDescriptionKey];
    return [NSError errorWithDomain:@"mopub.com" code:statusCode userInfo:errorInfo];
}

- (NSURLRequest *)adRequestForURL:(NSURL *)URL
{
    NSMutableURLRequest *request = [[MPCoreInstanceProvider sharedProvider] buildConfiguredURLRequestWithURL:URL];
    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    [request setTimeoutInterval:kRequestTimeoutInterval];
    return request;
}

@end
