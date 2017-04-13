//
//  MPNativePositionSource.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativePositionSource.h"
#import "MPConstants.h"
#import "MPIdentityProvider.h"
#import "MPAdPositioning.h"
#import "MPClientAdPositioning.h"
#import "MPLogging.h"
#import "MPNativePositionResponseDeserializer.h"
#import "MPAPIEndpoints.h"

static NSString * const kPositioningSourceErrorDomain = @"com.mopub.iossdk.positioningsource";
static const NSTimeInterval kMaximumRetryInterval = 60.0;
static const NSTimeInterval kMinimumRetryInterval = 1.0;
static const CGFloat kRetryIntervalBackoffMultiplier = 2.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativePositionSource () <NSURLConnectionDataDelegate>

@property (nonatomic, assign) BOOL loading;
@property (nonatomic, copy) NSString *adUnitIdentifier;
@property (nonatomic, strong) NSURLConnection *connection;
@property (nonatomic, strong) NSMutableData *data;
@property (nonatomic, copy) void (^completionHandler)(MPAdPositioning *positioning, NSError *error);
@property (nonatomic, assign) NSTimeInterval maximumRetryInterval;
@property (nonatomic, assign) NSTimeInterval minimumRetryInterval;
@property (nonatomic, assign) NSTimeInterval retryInterval;
@property (nonatomic, assign) NSUInteger retryCount;

- (NSURL *)serverURLWithAdUnitIdentifier:(NSString *)identifier;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPNativePositionSource

- (id)init
{
    if (self) {
        self.maximumRetryInterval = kMaximumRetryInterval;
        self.minimumRetryInterval = kMinimumRetryInterval;
        self.retryInterval = self.minimumRetryInterval;
    }
    return self;
}

- (void)dealloc
{
    [_connection cancel];
}

#pragma mark - Public

- (void)loadPositionsWithAdUnitIdentifier:(NSString *)identifier completionHandler:(void (^)(MPAdPositioning *positioning, NSError *error))completionHandler
{
    NSAssert(completionHandler != nil, @"A completion handler is required to load positions.");

    if (![identifier length]) {
        NSError *invalidIDError = [NSError errorWithDomain:kPositioningSourceErrorDomain code:MPNativePositionSourceInvalidAdUnitIdentifier userInfo:nil];
        completionHandler(nil, invalidIDError);
        return;
    }

    self.adUnitIdentifier = identifier;
    self.completionHandler = completionHandler;
    self.retryCount = 0;
    self.retryInterval = self.minimumRetryInterval;

    MPLogInfo(@"Requesting ad positions for native ad unit (%@).", identifier);

    NSURLRequest *request = [NSURLRequest requestWithURL:[self serverURLWithAdUnitIdentifier:identifier]];
    [self.connection cancel];
    [self.data setLength:0];
    self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
}

- (void)cancel
{
    // Cancel any connection currently in flight.
    [self.connection cancel];

    // Cancel any queued retry requests.
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

#pragma mark - Internal

- (NSURL *)serverURLWithAdUnitIdentifier:(NSString *)identifier
{
    NSString *URLString = [NSString stringWithFormat:@"%@?id=%@&v=%@&nsv=%@&udid=%@",
                           [MPAPIEndpoints baseURLStringWithPath:MOPUB_API_PATH_NATIVE_POSITIONING testing:NO],
                           identifier,
                           MP_SERVER_VERSION,
                           MP_SDK_VERSION,
                           [MPIdentityProvider identifier]];
    return [NSURL URLWithString:URLString];
}

- (void)retryLoadingPositions
{
    self.retryCount++;

    MPLogInfo(@"Retrying positions (retry attempt #%lu).", (unsigned long)self.retryCount);

    NSURLRequest *request = [NSURLRequest requestWithURL:[self serverURLWithAdUnitIdentifier:self.adUnitIdentifier]];
    [self.connection cancel];
    [self.data setLength:0];
    self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
}

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    if (self.retryInterval >= self.maximumRetryInterval) {
        self.completionHandler(nil, error);
        self.completionHandler = nil;
    } else {
        [self performSelector:@selector(retryLoadingPositions) withObject:nil afterDelay:self.retryInterval];
        self.retryInterval = MIN(self.retryInterval * kRetryIntervalBackoffMultiplier, self.maximumRetryInterval);
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    if (!self.data) {
        self.data = [NSMutableData data];
    }

    [self.data appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    NSError *deserializationError = nil;
    MPClientAdPositioning *positioning = [[MPNativePositionResponseDeserializer deserializer] clientPositioningForData:self.data error:&deserializationError];

    if (deserializationError) {
        MPLogDebug(@"Position deserialization failed with error: %@", deserializationError);

        NSError *underlyingError = [[deserializationError userInfo] objectForKey:NSUnderlyingErrorKey];
        if ([underlyingError code] == MPNativePositionResponseDataIsEmpty) {
            // Empty response data means the developer hasn't assigned any ad positions for the ad
            // unit. No point in retrying the request.
            self.completionHandler(nil, [NSError errorWithDomain:kPositioningSourceErrorDomain code:MPNativePositionSourceEmptyResponse userInfo:nil]);
            self.completionHandler = nil;
        } else if (self.retryInterval >= self.maximumRetryInterval) {
            self.completionHandler(nil, deserializationError);
            self.completionHandler = nil;
        } else {
            [self performSelector:@selector(retryLoadingPositions) withObject:nil afterDelay:self.retryInterval];
            self.retryInterval = MIN(self.retryInterval * kRetryIntervalBackoffMultiplier, self.maximumRetryInterval);
        }

        return;
    }

    self.completionHandler(positioning, nil);
    self.completionHandler = nil;
}

@end
