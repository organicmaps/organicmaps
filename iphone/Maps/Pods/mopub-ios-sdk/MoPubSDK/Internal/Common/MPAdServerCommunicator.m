//
//  MPAdServerCommunicator.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdServerCommunicator.h"

#import "MoPub.h"
#import "MPAdConfiguration.h"
#import "MPAdServerKeys.h"
#import "MPAPIEndpoints.h"
#import "MPConsentManager.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "MPRateLimitManager.h"
#import "MPURLRequest.h"

// Multiple response JSON fields
static NSString * const kAdResponsesKey = @"ad-responses";
static NSString * const kAdResonsesMetadataKey = @"metadata";
static NSString * const kAdResonsesContentKey = @"content";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerCommunicator ()

@property (nonatomic, assign, readwrite) BOOL loading;
@property (nonatomic, strong) NSURLSessionTask * task;
@property (nonatomic, strong) NSDictionary *responseHeaders;
@property (nonatomic) NSArray *topLevelJsonKeys;

@property (nonatomic, readonly) BOOL isRateLimited;

@end

@interface MPAdServerCommunicator (Consent)

/**
 Removes all ads.mopub.com cookies that are presently saved in NSHTTPCookieStorage to avoid inadvertently
 sending personal data across the wire via cookies. This method is expected to be called every ad request.
 */
- (void)removeAllMoPubCookies;

/**
 Handles all server-side consent overrides, and strips them out of the response JSON
 so that they are not propagated to the rest of the responses.
 @param serverResponseJson Top-level JSON response from the server
 @return Top-level JSON response stripped of all consent override fields
 */
- (NSDictionary *)handleConsentOverrides:(NSDictionary *)serverResponseJson;
@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdServerCommunicator

- (id)initWithDelegate:(id<MPAdServerCommunicatorDelegate>)delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;
        _topLevelJsonKeys = @[kNextUrlMetadataKey, kFormatMetadataKey];
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
    if (self.isRateLimited) {
        [self didFailWithError:[NSError tooManyRequests]];
        return;
    }

    [self cancel];

    // Delete any cookies previous creatives have set before starting the load
    [self removeAllMoPubCookies];

    // Check to be sure the SDK is initialized before starting the request
    if (!MoPub.sharedInstance.isSdkInitialized) {
        [self failLoadForSDKInit];
        return;
    }

    // Generate request
    MPURLRequest * request = [[MPURLRequest alloc] initWithURL:URL];
    MPLogEvent([MPLogEvent adRequestedWithRequest:request]);

    __weak __typeof__(self) weakSelf = self;
    self.task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * data, NSHTTPURLResponse * response) {
        // Capture strong self for the duration of this block.
        __typeof__(self) strongSelf = weakSelf;

        // Handle the response.
        [strongSelf didFinishLoadingWithData:data];
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

- (void)sendBeforeLoadUrlWithConfiguration:(MPAdConfiguration *)configuration
{
    if (configuration.beforeLoadURL != nil) {
        MPURLRequest * request = [MPURLRequest requestWithURL:configuration.beforeLoadURL];
        [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
            MPLogDebug(@"Successfully sent before load URL");
        } errorHandler:^(NSError * _Nonnull error) {
            MPLogInfo(@"Failed to send before load URL");
        }];
    }
}

- (void)sendAfterLoadUrlWithConfiguration:(MPAdConfiguration *)configuration
                      adapterLoadDuration:(NSTimeInterval)duration
                        adapterLoadResult:(MPAfterLoadResult)result
{
    NSArray * afterLoadUrls = [configuration afterLoadUrlsWithLoadDuration:duration loadResult:result];

    for (NSURL * afterLoadUrl in afterLoadUrls) {
        MPURLRequest * request = [MPURLRequest requestWithURL:afterLoadUrl];
        [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
            MPLogDebug(@"Successfully sent after load URL: %@", afterLoadUrl);
        } errorHandler:^(NSError * _Nonnull error) {
            MPLogDebug(@"Failed to send after load URL: %@", afterLoadUrl);
        }];
    }
}

- (BOOL)isRateLimited {
    return [[MPRateLimitManager sharedInstance] isRateLimitedForAdUnitId:self.delegate.adUnitId];
}

- (void)failLoadForSDKInit {
    NSError *error = [NSError adLoadFailedBecauseSdkNotInitialized];
    MPLogEvent([MPLogEvent error:error message:nil]);
    [self didFailWithError:error];
}

#pragma mark - Handlers

- (void)didFailWithError:(NSError *)error {
    // Do not record a logging event if we failed.
    self.loading = NO;
    [self.delegate communicatorDidFailWithError:error];
}

- (void)didFinishLoadingWithData:(NSData *)data {
    // In the event that the @c adUnitIdUsedForConsent from @c MPConsentManager is @c nil or malformed,
    // we should populate it with this known good adunit ID. This is to cover any edge case where the
    // publisher manages to initialize with no adunit ID or a malformed adunit ID.
    // It is known good since this is the success callback from the ad request.
    [MPConsentManager.sharedManager setAdUnitIdUsedForConsent:self.delegate.adUnitId isKnownGood:YES];

    // Headers from the original HTTP response are intentionally ignored as laid out
    // by the Client Side Waterfall design doc.
    //
    // The response data is a JSON payload conforming to the structure:
    // {
    //     "ad-responses": [
    //                      {
    //                          "metadata": {
    //                              "adm": "some advanced bidding payload",
    //                              "x-ad-timeout-ms": 5000,
    //                              "x-adtype": "rewarded_video",
    //                          },
    //                          "content": "Ad markup goes here"
    //                      }
    //                      ],
    //     "x-next-url": "https:// ..."
    // }

    NSError * error = nil;
    NSDictionary * json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
    if (error) {
        NSError * parseError = [NSError adResponseFailedToParseWithError:error];
        MPLogEvent([MPLogEvent error:parseError message:nil]);
        [self didFailWithError:parseError];
        return;
    }

    MPLogEvent([MPLogEvent adRequestReceivedResponse:json]);

    // Handle ad server overrides and strip them out of the top level JSON response.
    json = [self handleAdResponseOverrides:json];

    // Add top level json attributes to each ad server response so MPAdConfiguration contains
    // all attributes for an ad response.
    NSArray *responses = [self getFlattenJsonResponses:json keys:self.topLevelJsonKeys];
    if (responses == nil) {
        NSError * noResponsesError = [NSError adResponsesNotFound];
        MPLogEvent([MPLogEvent error:noResponsesError message:nil]);
        [self didFailWithError:noResponsesError];
        return;
    }

    // Attempt to parse each ad response JSON into its corresponding MPAdConfiguration object.
    NSMutableArray<MPAdConfiguration *> * configurations = [NSMutableArray arrayWithCapacity:responses.count];
    for (NSDictionary * responseJson in responses) {
        // The `metadata` field is required and must contain at least one entry. The `content` field is optional.
        // If there is a failure, this response should be ignored.
        NSDictionary * metadata = responseJson[kAdResonsesMetadataKey];
        NSData * content = [responseJson[kAdResonsesContentKey] dataUsingEncoding:NSUTF8StringEncoding];
        if (metadata == nil || (metadata != nil && metadata.count == 0)) {
            MPLogInfo(@"The metadata field is either non-existent or empty");
            continue;
        }

        MPAdConfiguration * configuration = [[MPAdConfiguration alloc] initWithMetadata:metadata data:content isFullscreenAd:self.delegate.isFullscreenAd];
        if (configuration != nil) {
            [configurations addObject:configuration];
        } else {
            MPLogInfo(@"Failed to generate configuration from\nmetadata:\n%@\nbody:\n%@", metadata, responseJson[kAdResonsesContentKey]);
        }
    }

    // Set up rate limiting (has no effect if backoffMs is 0)
    NSInteger backoffMs = [json[kBackoffMsKey] integerValue];
    NSString * backoffReason = json[kBackoffReasonKey];
    [[MPRateLimitManager sharedInstance] setRateLimitTimerWithAdUnitId:self.delegate.adUnitId
                                                          milliseconds:backoffMs
                                                                reason:backoffReason];

    self.loading = NO;
    [self.delegate communicatorDidReceiveAdConfigurations:configurations];
}

// Add top level json attributes to each ad server response so MPAdConfiguration contains
// all attributes for an ad response.
- (NSArray *)getFlattenJsonResponses:(NSDictionary *)json keys:(NSArray *)keys
{
    NSMutableArray *responses = json[kAdResponsesKey];
    if (responses == nil) {
        return nil;
    }

    NSMutableArray *flattenResponses = [NSMutableArray new];
    for (NSDictionary *response in responses) {
        NSMutableDictionary *flattenResponse = [response mutableCopy];
        flattenResponse[kAdResonsesMetadataKey] = [response[kAdResonsesMetadataKey] mutableCopy];

        for (NSString *key in keys) {
            flattenResponse[kAdResonsesMetadataKey][key] = json[key];
        }
        [flattenResponses addObject:flattenResponse];
    }
    return flattenResponses;
}

// Process any top level json attributes that trigger state changes within the SDK.
/**
 Handles all server-side overrides, and strips them out of the response JSON
 so that they are not propagated to the rest of the responses.
 @param serverResponseJson Top-level JSON response from the server
 @return Top-level JSON response stripped of all override fields
 */
- (NSDictionary *)handleAdResponseOverrides:(NSDictionary *)serverResponseJson {
    // Handle Consent
    NSMutableDictionary * json = [[self handleConsentOverrides:serverResponseJson] mutableCopy];

    // Handle the enabling of debug logging.
    NSNumber * debugLoggingEnabled = json[kEnableDebugLogging];
    if (debugLoggingEnabled != nil && [debugLoggingEnabled boolValue]) {
        MPLogInfo(@"Debug logging enabled");
        MPLogging.consoleLogLevel = MPBLogLevelDebug;

        json[kEnableDebugLogging] = nil;
    }

    return json;
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

/**
 Handles all server-side consent overrides, and strips them out of the response JSON
 so that they are not propagated to the rest of the responses.
 @param serverResponseJson Top-level JSON response from the server
 @return Top-level JSON response stripped of all consent override fields
 */
- (NSDictionary *)handleConsentOverrides:(NSDictionary *)serverResponseJson {
    // Nothing to handle.
    if (serverResponseJson == nil) {
        return nil;
    }

    // Handle the consent overrides
    [[MPConsentManager sharedManager] forceStatusShouldForceExplicitNo:[serverResponseJson[kForceExplicitNoKey] boolValue]
                                               shouldInvalidateConsent:[serverResponseJson[kInvalidateConsentKey] boolValue]
                                                shouldReacquireConsent:[serverResponseJson[kReacquireConsentKey] boolValue]
                                          shouldForceGDPRApplicability:[serverResponseJson[kForceGDPRAppliesKey] boolValue]
                                                   consentChangeReason:serverResponseJson[kConsentChangedReasonKey]
                                                       shouldBroadcast:YES];

    // Strip out the consent overrides
    NSMutableDictionary * parsedResponseJson = [serverResponseJson mutableCopy];
    parsedResponseJson[kForceExplicitNoKey] = nil;
    parsedResponseJson[kInvalidateConsentKey] = nil;
    parsedResponseJson[kReacquireConsentKey] = nil;
    parsedResponseJson[kForceGDPRAppliesKey] = nil;
    parsedResponseJson[kConsentChangedReasonKey] = nil;

    return parsedResponseJson;
}

@end
