//
//  MPAdServerCommunicator.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPAdServerCommunicator.h"

#import "MoPub.h"
#import "MPAdConfiguration.h"
#import "MPAPIEndpoints.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "MPURLRequest.h"

// Ad response header
static NSString * const kAdResponseTypeHeaderKey = @"X-Ad-Response-Type";
static NSString * const kAdResponseTypeMultipleResponse = @"multi";

// Multiple response JSON fields
static NSString * const kMultiAdResponsesKey = @"ad-responses";
static NSString * const kMultiAdResponsesHeadersKey = @"headers";
static NSString * const kMultiAdResponsesBodyKey = @"body";
static NSString * const kMultiAdResponsesAdMarkupKey = @"adm";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerCommunicator ()

@property (nonatomic, assign, readwrite) BOOL loading;
@property (nonatomic, strong) NSURLSessionTask * task;

@end

@interface MPAdServerCommunicator (Consent)

/**
 Removes all ads.mopub.com cookies that are presently saved in NSHTTPCookieStorage to avoid inadvertently
 sending personal data across the wire via cookies. This method is expected to be called every ad request.
 */
- (void)removeAllMoPubCookies;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdServerCommunicator

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
    [self.task cancel];
}

#pragma mark - Public

- (void)loadURL:(NSURL *)URL
{
    [self cancel];

    // Delete any cookies previous creatives have set before starting the load
    [self removeAllMoPubCookies];

    // Check to be sure the SDK is initialized before starting the request
    if (!MoPub.sharedInstance.isSdkInitialized) {
        [self failLoadForSDKInit];
    }

    // Generate request
    MPURLRequest * request = [[MPURLRequest alloc] initWithURL:URL];

    __weak __typeof__(self) weakSelf = self;
    self.task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * data, NSHTTPURLResponse * response) {
        // Capture strong self for the duration of this block.
        __typeof__(self) strongSelf = weakSelf;

        // Status code indicates an error.
        if (response.statusCode >= 400) {
            [strongSelf didFailWithError:[strongSelf errorForStatusCode:response.statusCode]];
            return;
        }

        // Handle the response.
        [strongSelf didFinishLoadingWithData:data headers:response.allHeaderFields];

    } errorHandler:^(NSError * error) {
        // Capture strong self for the duration of this block.
        __typeof__(self) strongSelf = weakSelf;

        // Handle the error.
        [strongSelf didFailWithError:error];
    }];

    self.loading = YES;
}

- (void)cancel
{
    self.loading = NO;
    [self.task cancel];
    self.task = nil;
}

- (void)failLoadForSDKInit {
    MPLogError(@"Warning: Ad requested before initializing MoPub SDK. MoPub SDK 5.2.0 will require initializeSdkWithConfiguration:completion: to be called on MoPub.sharedInstance before attempting to load ads. Please update your integration as soon as possible.");
}

#pragma mark - Handlers

- (void)didFailWithError:(NSError *)error {
    // Do not record a logging event if we failed.
    self.loading = NO;
    [self.delegate communicatorDidFailWithError:error];
}

- (void)didFinishLoadingWithData:(NSData *)data headers:(NSDictionary *)headers {
    NSArray <MPAdConfiguration *> *configurations;
    // Single ad response
    if (![headers[kAdResponseTypeHeaderKey] isEqualToString:kAdResponseTypeMultipleResponse]) {
        MPAdConfiguration *configuration = [[MPAdConfiguration alloc] initWithHeaders:headers
                                                                                 data:data];
        configurations = @[configuration];
    }
    // Multiple ad responses
    else {
        // The response data is a JSON payload conforming to the structure:
        // ad-responses: [
        //   {
        //     headers: { x-adtype: html, ... },
        //     body: "<!DOCTYPE html> <html> <head> ... </html>",
        //     adm: "some ad markup"
        //   },
        //   ...
        // ]
        NSError * error = nil;
        NSDictionary * json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
        if (error) {
            MPLogError(@"Failed to parse multiple ad response JSON: %@", error.localizedDescription);
            self.loading = NO;
            [self.delegate communicatorDidFailWithError:error];
            return;
        }

        NSArray * responses = json[kMultiAdResponsesKey];
        if (responses == nil) {
            MPLogError(@"No ad responses");
            self.loading = NO;
            [self.delegate communicatorDidFailWithError:[MOPUBError errorWithCode:MOPUBErrorUnableToParseJSONAdResponse]];
            return;
        }

        MPLogInfo(@"There are %ld ad responses", responses.count);

        NSMutableArray<MPAdConfiguration *> * responseConfigurations = [NSMutableArray arrayWithCapacity:responses.count];
        for (NSDictionary * responseJson in responses) {
            NSDictionary * headers = responseJson[kMultiAdResponsesHeadersKey];
            NSData * body = [responseJson[kMultiAdResponsesBodyKey] dataUsingEncoding:NSUTF8StringEncoding];

            MPAdConfiguration * configuration = [[MPAdConfiguration alloc] initWithHeaders:headers data:body];
            if (configuration) {
                configuration.advancedBidPayload = responseJson[kMultiAdResponsesAdMarkupKey];
                [responseConfigurations addObject:configuration];
            }
            else {
                MPLogInfo(@"Failed to generate configuration from\nheaders:\n%@\nbody:\n%@", headers, responseJson[kMultiAdResponsesBodyKey]);
            }
        }

        configurations = [NSArray arrayWithArray:responseConfigurations];
    }

    self.loading = NO;
    [self.delegate communicatorDidReceiveAdConfigurations:configurations];
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

@end

#pragma mark - Consent

@implementation MPAdServerCommunicator (Consent)

- (void)removeAllMoPubCookies {
    // Make NSURL from base URL
    NSURL *moPubBaseURL = [NSURL URLWithString:[MPAPIEndpoints baseURL]];

    // Get array of cookies with the base URL, and delete each one
    NSArray <NSHTTPCookie *> * cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:moPubBaseURL];
    for (NSHTTPCookie * cookie in cookies) {
        [[NSHTTPCookieStorage sharedHTTPCookieStorage] deleteCookie:cookie];
    }
}

@end
