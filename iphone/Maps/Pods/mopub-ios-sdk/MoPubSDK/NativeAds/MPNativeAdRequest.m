//
//  MPNativeAdRequest.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdRequest.h"

#import "MPAdServerURLBuilder.h"
#import "MPCoreInstanceProvider.h"
#import "MPNativeAdError.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPLogging.h"
#import "MPImageDownloadQueue.h"
#import "MPConstants.h"
#import "MPNativeAdConstants.h"
#import "MPNativeCustomEventDelegate.h"
#import "MPNativeCustomEvent.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MOPUBNativeVideoCustomEvent.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "MPAdServerCommunicator.h"
#import "MPNativeAdRenderer.h"
#import "MPMoPubNativeCustomEvent.h"
#import "MPNativeAdRendererConfiguration.h"
#import "NSMutableArray+MPAdditions.h"
#import "MPStopwatch.h"
#import "MPTimer.h"
#import "MPError.h"
#import "NSDate+MPAdditions.h"
#import "NSError+MPAdditions.h"

static NSString * const kNativeAdErrorDomain = @"com.mopub.NativeAd";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativeAdRequest () <MPNativeCustomEventDelegate, MPAdServerCommunicatorDelegate>

@property (nonatomic, copy) NSString *adUnitId;
@property (nonatomic) NSArray *rendererConfigurations;
@property (nonatomic, strong) NSURL *URL;
@property (nonatomic, strong) MPAdServerCommunicator *communicator;
@property (nonatomic, copy) MPNativeAdRequestHandler completionHandler;
@property (nonatomic, strong) MPNativeCustomEvent *nativeCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *adConfiguration;
@property (nonatomic, strong) NSMutableArray<MPAdConfiguration *> *remainingConfigurations;
@property (nonatomic) id<MPNativeAdRenderer> customEventRenderer;
@property (nonatomic, assign) BOOL loading;
@property (nonatomic, strong) MPTimer *timeoutTimer;
@property (nonatomic, strong) MPStopwatch *loadStopwatch;
@property (nonatomic, strong) NSURL *mostRecentlyLoadedURL;  // ADF-4286: avoid infinite ad reloads

@end

@implementation MPNativeAdRequest

- (id)initWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations
{
    self = [super init];
    if (self) {
        _adUnitId = [identifier copy];
        _communicator = [[MPAdServerCommunicator alloc] initWithDelegate:self];
        _rendererConfigurations = rendererConfigurations;
        _loadStopwatch = MPStopwatch.new;
    }
    return self;
}

- (void)dealloc
{
    [_communicator cancel];
    [_communicator setDelegate:nil];
    [_nativeCustomEvent setDelegate:nil];
}

#pragma mark - Public

+ (MPNativeAdRequest *)requestWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations
{
    return [[self alloc] initWithAdUnitIdentifier:identifier rendererConfigurations:rendererConfigurations];
}

- (void)startWithCompletionHandler:(MPNativeAdRequestHandler)handler
{
    if (handler) {
        self.URL = [MPAdServerURLBuilder URLWithAdUnitID:self.adUnitId
                                               targeting:self.targeting
                                           desiredAssets:[self.targeting.desiredAssets allObjects]
                                             viewability:NO];

        [self assignCompletionHandler:handler];

        [self loadAdWithURL:self.URL];
    } else {
        MPLogInfo(@"Native Ad Request did not start - requires completion handler block.");
    }
}

- (void)startForAdSequence:(NSInteger)adSequence withCompletionHandler:(MPNativeAdRequestHandler)handler
{
    if (handler) {
        self.URL = [MPAdServerURLBuilder URLWithAdUnitID:self.adUnitId
                                               targeting:self.targeting
                                           desiredAssets:[self.targeting.desiredAssets allObjects]
                                              adSequence:adSequence
                                             viewability:NO];

        [self assignCompletionHandler:handler];

        [self loadAdWithURL:self.URL];
    } else {
        MPLogInfo(@"Native Ad Request did not start - requires completion handler block.");
    }
}

#pragma mark - Private

- (void)assignCompletionHandler:(MPNativeAdRequestHandler)handler
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-retain-cycles"
    // we explicitly create a block retain cycle here to prevent self from being deallocated if the developer
    // removes his strong reference to the request. This retain cycle is broken in
    // - (void)completeAdRequestWithAdObject:(MPNativeAd *)adObject error:(NSError *)error
    // when self.completionHandler is set to nil.
    self.completionHandler = ^(MPNativeAdRequest *request, MPNativeAd *response, NSError *error) {
        handler(self, response, error);
    };
#pragma clang diagnostic pop
}

- (void)loadAdWithURL:(NSURL *)URL
{
    MPLogAdEvent(MPLogEvent.adLoadAttempt, self.adUnitId);

    if (self.loading) {
        MPLogInfo(@"Native ad request is already loading an ad. Wait for previous load to finish.");
        return;
    }

    self.loading = YES;
    self.mostRecentlyLoadedURL = URL;
    [self.communicator loadURL:URL];
}

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration
{
    if (configuration.customEventClass) {
        MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);
    }

     [self startTimeoutTimer];

    // For MoPub native ads, set the classData to be the adResponseData
    if ((configuration.customEventClass == [MPMoPubNativeCustomEvent class]) || (configuration.customEventClass == [MOPUBNativeVideoCustomEvent class])) {
        NSError *error;
        NSMutableDictionary *classData = [NSJSONSerialization mp_JSONObjectWithData:configuration.adResponseData
                                                                            options:0
                                                                   clearNullObjects:YES
                                                                              error:&error];
        if (configuration.customEventClass == [MOPUBNativeVideoCustomEvent class]) {
            classData[kNativeAdConfigKey] = [[MOPUBNativeVideoAdConfigValues alloc] initWithPlayVisiblePercent:configuration.nativeVideoPlayVisiblePercent
                                                                                           pauseVisiblePercent:configuration.nativeVideoPauseVisiblePercent
                                                                                    impressionMinVisiblePixels:configuration.nativeImpressionMinVisiblePixels
                                                                                   impressionMinVisiblePercent:configuration.nativeImpressionMinVisiblePercent
                                                                                   impressionMinVisibleSeconds:configuration.nativeImpressionMinVisibleTimeInterval
                                                                                              maxBufferingTime:configuration.nativeVideoMaxBufferingTime
                                                                                                      trackers:configuration.vastVideoTrackers];
        } else if (configuration.customEventClass == [MPMoPubNativeCustomEvent class]) {
            classData[kNativeAdConfigKey] = [[MPNativeAdConfigValues alloc] initWithImpressionMinVisiblePixels:configuration.nativeImpressionMinVisiblePixels
                                                                                   impressionMinVisiblePercent:configuration.nativeImpressionMinVisiblePercent
                                                                                   impressionMinVisibleSeconds:configuration.nativeImpressionMinVisibleTimeInterval];
        }

        // Additional information to be passed to the MoPub native custom events
        // for the purposes of logging.
        classData[kNativeAdUnitId] = self.adUnitId;
        classData[kNativeAdDspName] = nil; // Placeholder for future feature
        classData[kNativeAdDspCreativeId] = configuration.dspCreativeId;

        configuration.customEventClassData = classData;
    }

    // See if we have a renderer that we can use for the custom event now so we can fail early.
    NSString *customEventClassName = NSStringFromClass(configuration.customEventClass);
    MPNativeAdRendererConfiguration *customEventRendererConfig = nil;

    for (MPNativeAdRendererConfiguration *rendererConfig in self.rendererConfigurations) {
        NSArray *supportedCustomEvents = rendererConfig.supportedCustomEvents;

        if ([supportedCustomEvents containsObject:customEventClassName]) {
            customEventRendererConfig = rendererConfig;
            break;
        }
    }

    if (customEventRendererConfig == nil) {
        NSString * noRendererErrorMessage = [NSString stringWithFormat:@"Could not find renderer configuration for custom event class: %@", NSStringFromClass(configuration.customEventClass)];
        NSError * noRendererError = [NSError errorWithCode:MOPUBErrorNoRenderer localizedDescription:noRendererErrorMessage];
        MPLogEvent([MPLogEvent error:noRendererError message:nil]);

        [self nativeCustomEvent:nil didFailToLoadAdWithError:noRendererError];
        return;
    }
    // Create a renderer from the config.
    self.customEventRenderer = [[customEventRendererConfig.rendererClass alloc] initWithRendererSettings:customEventRendererConfig.rendererSettings];

    MPNativeCustomEvent *customEvent = [[configuration.customEventClass alloc] init];
    if (![customEvent isKindOfClass:[MPNativeCustomEvent class]]) {
        NSString * invalidCustomEventErrorMessage = [NSString stringWithFormat:@"Custom Event Class: %@ does not extend MPNativeCustomEvent", NSStringFromClass(configuration.customEventClass)];
        NSError * invalidCustomEventError = [NSError errorWithCode:MOPUBErrorNoRenderer localizedDescription:invalidCustomEventErrorMessage];
        MPLogEvent([MPLogEvent error:invalidCustomEventError message:nil]);

        [self nativeCustomEvent:nil didFailToLoadAdWithError:invalidCustomEventError];
        return;
    }
    customEvent.delegate = self;
    customEvent.localExtras = self.targeting.localExtras;
    self.nativeCustomEvent = customEvent;

    [self.nativeCustomEvent requestAdWithCustomEventInfo:configuration.customEventClassData adMarkup:configuration.advancedBidPayload];
}

- (void)failAdRequest {
    if (self.adConfiguration.nextURL == nil || [[self.adConfiguration.nextURL absoluteString] length] == 0) {
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForInvalidAdServerResponse(nil)];
        return;
    }

    self.loading = NO;
    [self loadAdWithURL:self.adConfiguration.nextURL];
}

- (void)completeAdRequestWithAdObject:(MPNativeAd *)adObject error:(NSError *)error
{
    [self didStopLoading];

    self.loading = NO;
    self.remainingConfigurations = nil;

    adObject.renderer = self.customEventRenderer;
    adObject.configuration = self.adConfiguration;
    adObject.adUnitID = self.adUnitId;

    if ([(id)adObject.adAdapter respondsToSelector:@selector(setAdConfiguration:)]) {
        [(id)adObject.adAdapter performSelector:@selector(setAdConfiguration:) withObject:self.adConfiguration];
    }

    if (error == nil) {
        MPLogAdEvent(MPLogEvent.adDidLoad, self.adUnitId);
    } else {
        MPLogAdEvent([MPLogEvent adFailedToLoadWithError:error], self.adUnitId);
    }

    if (self.completionHandler) {
        self.completionHandler(self, adObject, error);
        self.completionHandler = nil;
    }
}

- (void)fetchAdWithConfiguration:(MPAdConfiguration *)configuration {
    if (configuration.adUnitWarmingUp) {
        MPLogInfo(kMPWarmingUpErrorLogFormatWithAdUnitID, self.adUnitId);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForAdUnitWarmingUp()];
        return;
    }

    if ([configuration.adType isEqualToString:kAdTypeClear]) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitId);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNoInventory()];
        return;
    }

    MPLogInfo(@"Received data from MoPub to construct native ad.\n");

    // Notify Ad Server of the adapter load. This is fire and forget.
    [self.communicator sendBeforeLoadUrlWithConfiguration:configuration];

    // Start the stopwatch for the adapter load.
    [self.loadStopwatch start];

    [self getAdWithConfiguration:self.adConfiguration];
}

#pragma mark - <MPAdServerCommunicatorDelegate>

- (void)communicatorDidReceiveAdConfigurations:(NSArray<MPAdConfiguration *> *)configurations
{
    self.remainingConfigurations = [configurations mutableCopy];
    self.adConfiguration = [self.remainingConfigurations removeFirst];

    // There are no configurations to try. Consider this a clear response by the server.
    if (self.remainingConfigurations.count == 0 && self.adConfiguration == nil) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitId);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNoInventory()];
        return;
    }

    [self fetchAdWithConfiguration:self.adConfiguration];
}

- (void)communicatorDidFailWithError:(NSError *)error
{
    MPLogDebug(@"Error: Couldn't retrieve an ad from MoPub. Message: %@", error);

    [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNetworkConnectionError()];
}

- (BOOL)isFullscreenAd {
    return NO;
}

#pragma mark - <MPNativeCustomEventDelegate>

- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didLoadAd:(MPNativeAd *)adObject
{
    // Record the end of the adapter load and send off the fire and forget after-load-url tracker.
    NSTimeInterval duration = [self.loadStopwatch stop];
    [self.communicator sendAfterLoadUrlWithConfiguration:self.adConfiguration adapterLoadDuration:duration adapterLoadResult:MPAfterLoadResultAdLoaded];

    // Add the click tracker url from the header to our set.
    if (self.adConfiguration.clickTrackingURL) {
        [adObject.clickTrackerURLs addObject:self.adConfiguration.clickTrackingURL];
    }

    // Add the impression tracker url from the header to our set.
    if (self.adConfiguration.impressionTrackingURLs) {
        [adObject.impressionTrackerURLs addObjectsFromArray:self.adConfiguration.impressionTrackingURLs];
    }

    // Error if we don't have click trackers or impression trackers.
    if (adObject.clickTrackerURLs.count < 1 || adObject.impressionTrackerURLs.count < 1) {
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid ad trackers")];
    }
    else {
        [self completeAdRequestWithAdObject:adObject error:nil];
    }
}

- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didFailToLoadAdWithError:(NSError *)error
{
    // Record the end of the adapter load and send off the fire and forget after-load-url tracker
    // with the appropriate error code result.
    NSTimeInterval duration = [self.loadStopwatch stop];
    MPAfterLoadResult result = (error.isAdRequestTimedOutError ? MPAfterLoadResultTimeout : (event == nil ? MPAfterLoadResultMissingAdapter : MPAfterLoadResultError));
    [self.communicator sendAfterLoadUrlWithConfiguration:self.adConfiguration adapterLoadDuration:duration adapterLoadResult:result];

    // There are more ad configurations to try.
    if (self.remainingConfigurations.count > 0) {
        self.adConfiguration = [self.remainingConfigurations removeFirst];
        [self fetchAdWithConfiguration:self.adConfiguration];
    }
    // No more configurations to try. Fail over and let Ad Server get more ads
    else if (self.adConfiguration.nextURL != nil
             && [self.adConfiguration.nextURL isEqual:self.mostRecentlyLoadedURL] == false) {
        self.loading = NO;
        [self loadAdWithURL:self.adConfiguration.nextURL];
    }
    // Nothing left to try.
    else {
        self.loading = NO;

        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitId);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNoInventory()];
    }
}

- (void)startTimeoutTimer
{
    NSTimeInterval timeInterval = (self.adConfiguration && self.adConfiguration.adTimeoutInterval >= 0) ? self.adConfiguration.adTimeoutInterval : NATIVE_TIMEOUT_INTERVAL;

    if (timeInterval > 0) {
        self.timeoutTimer = [MPTimer timerWithTimeInterval:timeInterval
                                                    target:self
                                                  selector:@selector(timeout)
                                                   repeats:NO];
        [self.timeoutTimer scheduleNow];
    }
}

- (void)timeout
{
    NSError * error = [NSError errorWithCode:MOPUBErrorAdRequestTimedOut localizedDescription:@"Native ad request timed out"];
    [self nativeCustomEvent:self.nativeCustomEvent didFailToLoadAdWithError:error];
}

- (void)didStopLoading
{
    [self.timeoutTimer invalidate];
}

@end
