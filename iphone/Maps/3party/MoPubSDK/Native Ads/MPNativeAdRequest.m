//
//  MPNativeAdRequest.m
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPNativeAdRequest.h"

#import "MPAdServerURLBuilder.h"
#import "MPCoreInstanceProvider.h"
#import "MPNativeAdError.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPLogEvent.h"
#import "MPLogging.h"
#import "MPImageDownloadQueue.h"
#import "MPConstants.h"
#import "MPNativeAdConstants.h"
#import "MPNativeCustomEventDelegate.h"
#import "MPNativeCustomEvent.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MOPUBNativeVideoCustomEvent.h"
#import "MPInstanceProvider.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "MPAdServerCommunicator.h"
#import "MPNativeAdRenderer.h"
#import "MPMoPubNativeCustomEvent.h"
#import "MPNativeAdRendererConfiguration.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativeAdRequest () <MPNativeCustomEventDelegate, MPAdServerCommunicatorDelegate>

@property (nonatomic, copy) NSString *adUnitIdentifier;
@property (nonatomic) NSArray *rendererConfigurations;
@property (nonatomic, strong) NSURL *URL;
@property (nonatomic, strong) MPAdServerCommunicator *communicator;
@property (nonatomic, copy) MPNativeAdRequestHandler completionHandler;
@property (nonatomic, strong) MPNativeCustomEvent *nativeCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *adConfiguration;
@property (nonatomic) id<MPNativeAdRenderer> customEventRenderer;
@property (nonatomic, assign) BOOL loading;

@end

@implementation MPNativeAdRequest

- (id)initWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations
{
    self = [super init];
    if (self) {
        _adUnitIdentifier = [identifier copy];
        _communicator = [[MPCoreInstanceProvider sharedProvider] buildMPAdServerCommunicatorWithDelegate:self];
        _rendererConfigurations = rendererConfigurations;
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
        self.URL = [MPAdServerURLBuilder URLWithAdUnitID:self.adUnitIdentifier
                                                keywords:self.targeting.keywords
                                                location:self.targeting.location
                                    versionParameterName:@"nsv"
                                                 version:MP_SDK_VERSION
                                                 testing:NO
                                           desiredAssets:[self.targeting.desiredAssets allObjects]];

        [self assignCompletionHandler:handler];

        [self loadAdWithURL:self.URL];
    } else {
        MPLogWarn(@"Native Ad Request did not start - requires completion handler block.");
    }
}

- (void)startForAdSequence:(NSInteger)adSequence withCompletionHandler:(MPNativeAdRequestHandler)handler
{
    if (handler) {
        self.URL = [MPAdServerURLBuilder URLWithAdUnitID:self.adUnitIdentifier
                                                keywords:self.targeting.keywords
                                                location:self.targeting.location
                                    versionParameterName:@"nsv"
                                                 version:MP_SDK_VERSION
                                                 testing:NO
                                           desiredAssets:[self.targeting.desiredAssets allObjects]
                                              adSequence:adSequence];

        [self assignCompletionHandler:handler];

        [self loadAdWithURL:self.URL];
    } else {
        MPLogWarn(@"Native Ad Request did not start - requires completion handler block.");
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
    if (self.loading) {
        MPLogWarn(@"Native ad request is already loading an ad. Wait for previous load to finish.");
        return;
    }

    MPLogInfo(@"Starting ad request with URL: %@", self.URL);

    self.loading = YES;
    [self.communicator loadURL:URL];
}

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration
{
    if (configuration.customEventClass) {
        MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);
    }

    // For MoPub native ads, set the classData to be the adResponseData
    if ((configuration.customEventClass == [MPMoPubNativeCustomEvent class]) || (configuration.customEventClass == [MOPUBNativeVideoCustomEvent class])) {
        NSError *error;
        NSMutableDictionary *classData = [NSJSONSerialization mp_JSONObjectWithData:configuration.adResponseData
                                                                            options:0
                                                                   clearNullObjects:YES
                                                                              error:&error];
        if (configuration.customEventClass == [MOPUBNativeVideoCustomEvent class]) {
            [classData setObject:[[MOPUBNativeVideoAdConfigValues alloc]
                                  initWithPlayVisiblePercent:configuration.nativeVideoPlayVisiblePercent
                                  pauseVisiblePercent:configuration.nativeVideoPauseVisiblePercent
                                  impressionMinVisiblePercent:configuration.nativeVideoImpressionMinVisiblePercent
                                  impressionVisible:configuration.nativeVideoImpressionVisible
                                  maxBufferingTime:configuration.nativeVideoMaxBufferingTime
                                  trackers:configuration.nativeVideoTrackers] forKey:kNativeVideoAdConfigKey];
            MPAdConfigurationLogEventProperties *logEventProperties =
                [[MPAdConfigurationLogEventProperties alloc] initWithConfiguration:configuration];
            [classData setObject:logEventProperties forKey:kLogEventRequestPropertiesKey];
        }

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

    if (customEventRendererConfig) {
        // Create a renderer from the config.
        self.customEventRenderer = [[customEventRendererConfig.rendererClass alloc] initWithRendererSettings:customEventRendererConfig.rendererSettings];
        self.nativeCustomEvent = [[MPInstanceProvider sharedProvider] buildNativeCustomEventFromCustomClass:configuration.customEventClass delegate:self];
    } else {
        MPLogError(@"Could not find renderer configuration for custom event class: %@", NSStringFromClass(configuration.customEventClass));
    }

    if (self.nativeCustomEvent) {
        [self.nativeCustomEvent requestAdWithCustomEventInfo:configuration.customEventClassData];
    } else if ([[self.adConfiguration.failoverURL absoluteString] length]) {
        self.loading = NO;
        [self loadAdWithURL:self.adConfiguration.failoverURL];
    } else {
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForInvalidAdServerResponse(nil)];
    }
}

- (void)completeAdRequestWithAdObject:(MPNativeAd *)adObject error:(NSError *)error
{
    self.loading = NO;

    adObject.renderer = self.customEventRenderer;

    if ([(id)adObject.adAdapter respondsToSelector:@selector(setAdConfiguration:)]) {
        [(id)adObject.adAdapter performSelector:@selector(setAdConfiguration:) withObject:self.adConfiguration];
    }

    if (!error) {
        MPLogInfo(@"Successfully loaded native ad.");
    } else {
        MPLogError(@"Native ad failed to load with error: %@", error);
    }

    if (self.completionHandler) {
        self.completionHandler(self, adObject, error);
        self.completionHandler = nil;
    }
}

#pragma mark - <MPAdServerCommunicatorDelegate>

- (void)communicatorDidReceiveAdConfiguration:(MPAdConfiguration *)configuration
{
    self.adConfiguration = configuration;

    if (configuration.adUnitWarmingUp) {
        MPLogInfo(kMPWarmingUpErrorLogFormatWithAdUnitID, self.adUnitIdentifier);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForAdUnitWarmingUp()];
        return;
    }

    if ([configuration.networkType isEqualToString:kAdTypeClear]) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitIdentifier);
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNoInventory()];
        return;
    }

    MPLogInfo(@"Received data from MoPub to construct native ad.\n");
    [self getAdWithConfiguration:configuration];
}

- (void)communicatorDidFailWithError:(NSError *)error
{
    MPLogDebug(@"Error: Couldn't retrieve an ad from MoPub. Message: %@", error);

    [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForNetworkConnectionError()];
}

#pragma mark - <MPNativeCustomEventDelegate>

- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didLoadAd:(MPNativeAd *)adObject
{
    // Add the click tracker url from the header to our set.
    if (self.adConfiguration.clickTrackingURL) {
        [adObject.clickTrackerURLs addObject:self.adConfiguration.clickTrackingURL];
    }

    // Add the impression tracker url from the header to our set.
    if (self.adConfiguration.impressionTrackingURL) {
        [adObject.impressionTrackerURLs addObject:self.adConfiguration.impressionTrackingURL];
    }

    // Error if we don't have click trackers or impression trackers.
    if (adObject.clickTrackerURLs.count < 1 || adObject.impressionTrackerURLs.count < 1) {
        [self completeAdRequestWithAdObject:nil error:MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid ad trackers")];
    } else {
        [self completeAdRequestWithAdObject:adObject error:nil];
    }
}

- (void)nativeCustomEvent:(MPNativeCustomEvent *)event didFailToLoadAdWithError:(NSError *)error
{
    if ([[self.adConfiguration.failoverURL absoluteString] length]) {
        self.loading = NO;
        [self loadAdWithURL:self.adConfiguration.failoverURL];
    } else {
        [self completeAdRequestWithAdObject:nil error:error];
    }
}


@end
