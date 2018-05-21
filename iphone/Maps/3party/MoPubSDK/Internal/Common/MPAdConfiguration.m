//
//  MPAdConfiguration.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPAdConfiguration.h"

#import "MOPUBExperimentProvider.h"
#import "MPConsentAdServerKeys.h"
#import "MPConsentManager.h"
#import "MPConstants.h"
#import "MPLogging.h"
#import "MPRewardedVideoReward.h"
#import "MPViewabilityTracker.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "NSString+MPAdditions.h"

#if MP_HAS_NATIVE_PACKAGE
#import "MPVASTTrackingEvent.h"
#endif

NSString * const kAdTypeHeaderKey = @"X-Adtype";
NSString * const kAdUnitWarmingUpHeaderKey = @"X-Warmup";
NSString * const kClickthroughHeaderKey = @"X-Clickthrough";
NSString * const kCreativeIdHeaderKey = @"X-CreativeId";
NSString * const kCustomSelectorHeaderKey = @"X-Customselector";
NSString * const kCustomEventClassNameHeaderKey = @"X-Custom-Event-Class-Name";
NSString * const kCustomEventClassDataHeaderKey = @"X-Custom-Event-Class-Data";
NSString * const kFailUrlHeaderKey = @"X-Failurl";
NSString * const kHeightHeaderKey = @"X-Height";
NSString * const kImpressionTrackerHeaderKey = @"X-Imptracker";
NSString * const kLaunchpageHeaderKey = @"X-Launchpage";
NSString * const kNativeSDKParametersHeaderKey = @"X-Nativeparams";
NSString * const kNetworkTypeHeaderKey = @"X-Networktype";
NSString * const kRefreshTimeHeaderKey = @"X-Refreshtime";
NSString * const kAdTimeoutHeaderKey = @"X-AdTimeout";
NSString * const kScrollableHeaderKey = @"X-Scrollable";
NSString * const kWidthHeaderKey = @"X-Width";
NSString * const kDspCreativeIdKey = @"X-DspCreativeid";
NSString * const kPrecacheRequiredKey = @"X-PrecacheRequired";
NSString * const kIsVastVideoPlayerKey = @"X-VastVideoPlayer";

NSString * const kInterstitialAdTypeHeaderKey = @"X-Fulladtype";
NSString * const kOrientationTypeHeaderKey = @"X-Orientation";

NSString * const kNativeImpressionMinVisiblePixelsHeaderKey = @"X-Native-Impression-Min-Px"; // The pixels header takes priority over percentage, but percentage is left for backwards compatibility
NSString * const kNativeImpressionMinVisiblePercentHeaderKey = @"X-Impression-Min-Visible-Percent";
NSString * const kNativeImpressionVisibleMsHeaderKey = @"X-Impression-Visible-Ms";
NSString * const kNativeVideoPlayVisiblePercentHeaderKey = @"X-Play-Visible-Percent";
NSString * const kNativeVideoPauseVisiblePercentHeaderKey = @"X-Pause-Visible-Percent";
NSString * const kNativeVideoMaxBufferingTimeMsHeaderKey = @"X-Max-Buffer-Ms";
NSString * const kNativeVideoTrackersHeaderKey = @"X-Video-Trackers";

NSString * const kBannerImpressionVisableMsHeaderKey = @"X-Banner-Impression-Min-Ms";
NSString * const kBannerImpressionMinPixelHeaderKey = @"X-Banner-Impression-Min-Pixels";

NSString * const kAdTypeHtml = @"html";
NSString * const kAdTypeInterstitial = @"interstitial";
NSString * const kAdTypeMraid = @"mraid";
NSString * const kAdTypeClear = @"clear";
NSString * const kAdTypeNative = @"json";
NSString * const kAdTypeNativeVideo = @"json_video";

// rewarded video
NSString * const kRewardedVideoCurrencyNameHeaderKey = @"X-Rewarded-Video-Currency-Name";
NSString * const kRewardedVideoCurrencyAmountHeaderKey = @"X-Rewarded-Video-Currency-Amount";
NSString * const kRewardedVideoCompletionUrlHeaderKey = @"X-Rewarded-Video-Completion-Url";
NSString * const kRewardedCurrenciesHeaderKey = @"X-Rewarded-Currencies";

// rewarded playables
NSString * const kRewardedPlayableDurationHeaderKey = @"X-Rewarded-Duration";
NSString * const kRewardedPlayableRewardOnClickHeaderKey = @"X-Should-Reward-On-Click";

// native video
NSString * const kNativeVideoTrackerUrlMacro = @"%%VIDEO_EVENT%%";
NSString * const kNativeVideoTrackerEventsHeaderKey = @"events";
NSString * const kNativeVideoTrackerUrlsHeaderKey = @"urls";
NSString * const kNativeVideoTrackerEventDictionaryKey = @"event";
NSString * const kNativeVideoTrackerTextDictionaryKey = @"text";

// clickthrough experiment
NSString * const kClickthroughExperimentBrowserAgent = @"X-Browser-Agent";
static const NSInteger kMaximumVariantForClickthroughExperiment = 2;

// viewability
NSString * const kViewabilityDisableHeaderKey = @"X-Disable-Viewability";


@interface MPAdConfiguration ()

@property (nonatomic, copy) NSString *adResponseHTMLString;
@property (nonatomic, strong, readwrite) NSArray *availableRewards;
@property (nonatomic) MOPUBDisplayAgentType clickthroughExperimentBrowserAgent;

- (MPAdType)adTypeFromHeaders:(NSDictionary *)headers;
- (NSString *)networkTypeFromHeaders:(NSDictionary *)headers;
- (NSTimeInterval)refreshIntervalFromHeaders:(NSDictionary *)headers;
- (NSDictionary *)dictionaryFromHeaders:(NSDictionary *)headers forKey:(NSString *)key;
- (NSURL *)URLFromHeaders:(NSDictionary *)headers forKey:(NSString *)key;
- (Class)setUpCustomEventClassFromHeaders:(NSDictionary *)headers;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdConfiguration

- (id)initWithHeaders:(NSDictionary *)headers data:(NSData *)data
{
    self = [super init];
    if (self) {
        self.adResponseData = data;

        self.adType = [self adTypeFromHeaders:headers];

        self.adUnitWarmingUp = [[headers objectForKey:kAdUnitWarmingUpHeaderKey] boolValue];

        self.networkType = [self networkTypeFromHeaders:headers];
        self.networkType = self.networkType ? self.networkType : @"";

        self.preferredSize = CGSizeMake([[headers objectForKey:kWidthHeaderKey] floatValue],
                                        [[headers objectForKey:kHeightHeaderKey] floatValue]);

        self.clickTrackingURL = [self URLFromHeaders:headers
                                              forKey:kClickthroughHeaderKey];
        self.impressionTrackingURL = [self URLFromHeaders:headers
                                                   forKey:kImpressionTrackerHeaderKey];
        self.failoverURL = [self URLFromHeaders:headers
                                         forKey:kFailUrlHeaderKey];
        self.interceptURLPrefix = [self URLFromHeaders:headers
                                                forKey:kLaunchpageHeaderKey];

        self.scrollable = [[headers objectForKey:kScrollableHeaderKey] boolValue];
        self.refreshInterval = [self refreshIntervalFromHeaders:headers];
        self.adTimeoutInterval = [self timeIntervalFromHeaders:headers forKey:kAdTimeoutHeaderKey];


        self.nativeSDKParameters = [self dictionaryFromHeaders:headers
                                                        forKey:kNativeSDKParametersHeaderKey];
        self.customSelectorName = [headers objectForKey:kCustomSelectorHeaderKey];

        self.orientationType = [self orientationTypeFromHeaders:headers];

        self.customEventClass = [self setUpCustomEventClassFromHeaders:headers];

        self.customEventClassData = [self customEventClassDataFromHeaders:headers];

        self.dspCreativeId = [headers objectForKey:kDspCreativeIdKey];

        self.precacheRequired = [[headers objectForKey:kPrecacheRequiredKey] boolValue];

        self.isVastVideoPlayer = [[headers objectForKey:kIsVastVideoPlayerKey] boolValue];

        self.creationTimestamp = [NSDate date];

        self.creativeId = [headers objectForKey:kCreativeIdHeaderKey];

        self.headerAdType = [headers objectForKey:kAdTypeHeaderKey];

        self.nativeVideoPlayVisiblePercent = [self percentFromHeaders:headers forKey:kNativeVideoPlayVisiblePercentHeaderKey];

        self.nativeVideoPauseVisiblePercent = [self percentFromHeaders:headers forKey:kNativeVideoPauseVisiblePercentHeaderKey];

        self.nativeImpressionMinVisiblePixels = [[self adAmountFromHeaders:headers key:kNativeImpressionMinVisiblePixelsHeaderKey] floatValue];

        self.nativeImpressionMinVisiblePercent = [self percentFromHeaders:headers forKey:kNativeImpressionMinVisiblePercentHeaderKey];

        self.nativeImpressionMinVisibleTimeInterval = [self timeIntervalFromMsHeaders:headers forKey:kNativeImpressionVisibleMsHeaderKey];

        self.nativeVideoMaxBufferingTime = [self timeIntervalFromMsHeaders:headers forKey:kNativeVideoMaxBufferingTimeMsHeaderKey];
#if MP_HAS_NATIVE_PACKAGE
        self.nativeVideoTrackers = [self nativeVideoTrackersFromHeaders:headers key:kNativeVideoTrackersHeaderKey];
#endif

        self.impressionMinVisibleTimeInSec = [self timeIntervalFromMsHeaders:headers forKey:kBannerImpressionVisableMsHeaderKey];
        self.impressionMinVisiblePixels = [[self adAmountFromHeaders:headers key:kBannerImpressionMinPixelHeaderKey] floatValue];

        // rewarded video

        // Attempt to parse the multiple currency header first since this will take
        // precedence over the older single currency approach.
        self.availableRewards = [self parseAvailableRewardsFromHeaders:headers];
        if (self.availableRewards != nil) {
            // Multiple currencies exist. We will select the first entry in the list
            // as the default selected reward.
            if (self.availableRewards.count > 0) {
                self.selectedReward = self.availableRewards[0];
            }
            // In the event that the list of available currencies is empty, we will
            // follow the behavior from the single currency approach and create an unspecified reward.
            else {
                MPRewardedVideoReward * defaultReward = [[MPRewardedVideoReward alloc] initWithCurrencyType:kMPRewardedVideoRewardCurrencyTypeUnspecified amount:@(kMPRewardedVideoRewardCurrencyAmountUnspecified)];
                self.availableRewards = [NSArray arrayWithObject:defaultReward];
                self.selectedReward = defaultReward;
            }
        }
        // Multiple currencies are not available; attempt to process single currency
        // headers.
        else {
            NSString *currencyName = [headers objectForKey:kRewardedVideoCurrencyNameHeaderKey] ?: kMPRewardedVideoRewardCurrencyTypeUnspecified;

            NSNumber *currencyAmount = [self adAmountFromHeaders:headers key:kRewardedVideoCurrencyAmountHeaderKey];
            if (currencyAmount.integerValue <= 0) {
                currencyAmount = @(kMPRewardedVideoRewardCurrencyAmountUnspecified);
            }

            MPRewardedVideoReward * reward = [[MPRewardedVideoReward alloc] initWithCurrencyType:currencyName amount:currencyAmount];
            self.availableRewards = [NSArray arrayWithObject:reward];
            self.selectedReward = reward;
        }

        self.rewardedVideoCompletionUrl = [headers objectForKey:kRewardedVideoCompletionUrlHeaderKey];

        // rewarded playables
        self.rewardedPlayableDuration = [self timeIntervalFromHeaders:headers forKey:kRewardedPlayableDurationHeaderKey];
        self.rewardedPlayableShouldRewardOnClick = [[headers objectForKey:kRewardedPlayableRewardOnClickHeaderKey] boolValue];

        // clickthrough experiment
        self.clickthroughExperimentBrowserAgent = [self clickthroughExperimentVariantFromHeaders:headers forKey:kClickthroughExperimentBrowserAgent];
        [MOPUBExperimentProvider setDisplayAgentFromAdServer:self.clickthroughExperimentBrowserAgent];

        // viewability
        NSString * disabledViewabilityValue = [headers objectForKey:kViewabilityDisableHeaderKey];
        NSNumber * disabledViewabilityVendors = disabledViewabilityValue != nil ? [disabledViewabilityValue safeIntegerValue] : nil;
        if (disabledViewabilityVendors != nil &&
            [disabledViewabilityVendors integerValue] >= MPViewabilityOptionNone &&
            [disabledViewabilityVendors integerValue] <= MPViewabilityOptionAll) {
            MPViewabilityOption vendorsToDisable = (MPViewabilityOption)([disabledViewabilityVendors integerValue]);
            [MPViewabilityTracker disableViewability:vendorsToDisable];
        }

        // consent
        [[MPConsentManager sharedManager] forceStatusShouldForceExplicitNo:[headers[kForceExplicitNoKey] boolValue]
                                                   shouldInvalidateConsent:[headers[kInvalidateConsentKey] boolValue]
                                                    shouldReacquireConsent:[headers[kReacquireConsentKey] boolValue]
                                                       consentChangeReason:headers[kConsentChangedReasonKey]
                                                   shouldBroadcast:YES];
    }
    return self;
}

- (Class)setUpCustomEventClassFromHeaders:(NSDictionary *)headers
{
    NSString *customEventClassName = [headers objectForKey:kCustomEventClassNameHeaderKey];

    NSMutableDictionary *convertedCustomEvents = [NSMutableDictionary dictionary];
    if (self.adType == MPAdTypeBanner) {
        [convertedCustomEvents setObject:@"MPGoogleAdMobBannerCustomEvent" forKey:@"admob_native"];
        [convertedCustomEvents setObject:@"MPMillennialBannerCustomEvent" forKey:@"millennial_native"];
        [convertedCustomEvents setObject:@"MPHTMLBannerCustomEvent" forKey:@"html"];
        [convertedCustomEvents setObject:@"MPMRAIDBannerCustomEvent" forKey:@"mraid"];
        [convertedCustomEvents setObject:@"MOPUBNativeVideoCustomEvent" forKey:@"json_video"];
        [convertedCustomEvents setObject:@"MPMoPubNativeCustomEvent" forKey:@"json"];
    } else if (self.adType == MPAdTypeInterstitial) {
        [convertedCustomEvents setObject:@"MPGoogleAdMobInterstitialCustomEvent" forKey:@"admob_full"];
        [convertedCustomEvents setObject:@"MPMillennialInterstitialCustomEvent" forKey:@"millennial_full"];
        [convertedCustomEvents setObject:@"MPHTMLInterstitialCustomEvent" forKey:@"html"];
        [convertedCustomEvents setObject:@"MPMRAIDInterstitialCustomEvent" forKey:@"mraid"];
        [convertedCustomEvents setObject:@"MPMoPubRewardedVideoCustomEvent" forKey:@"rewarded_video"];
        [convertedCustomEvents setObject:@"MPMoPubRewardedPlayableCustomEvent" forKey:@"rewarded_playable"];
    }
    if ([convertedCustomEvents objectForKey:self.networkType]) {
        customEventClassName = [convertedCustomEvents objectForKey:self.networkType];
    }

    Class customEventClass = NSClassFromString(customEventClassName);

    if (customEventClassName && !customEventClass) {
        MPLogWarn(@"Could not find custom event class named %@", customEventClassName);
    }

    return customEventClass;
}



- (NSDictionary *)customEventClassDataFromHeaders:(NSDictionary *)headers
{
    NSDictionary *result = [self dictionaryFromHeaders:headers forKey:kCustomEventClassDataHeaderKey];
    if (!result) {
        result = [self dictionaryFromHeaders:headers forKey:kNativeSDKParametersHeaderKey];
    }
    return result;
}


- (BOOL)hasPreferredSize
{
    return (self.preferredSize.width > 0 && self.preferredSize.height > 0);
}

- (NSString *)adResponseHTMLString
{
    if (!_adResponseHTMLString) {
        self.adResponseHTMLString = [[NSString alloc] initWithData:self.adResponseData
                                                           encoding:NSUTF8StringEncoding];
    }

    return _adResponseHTMLString;
}

- (NSString *)clickDetectionURLPrefix
{
    return self.interceptURLPrefix.absoluteString ? self.interceptURLPrefix.absoluteString : @"";
}

#pragma mark - Private

- (MPAdType)adTypeFromHeaders:(NSDictionary *)headers
{
    NSString *adTypeString = [headers objectForKey:kAdTypeHeaderKey];

    if ([adTypeString isEqualToString:@"interstitial"] || [adTypeString isEqualToString:@"rewarded_video"] || [adTypeString isEqualToString:@"rewarded_playable"]) {
        return MPAdTypeInterstitial;
    } else if (adTypeString &&
               [headers objectForKey:kOrientationTypeHeaderKey]) {
        return MPAdTypeInterstitial;
    } else if (adTypeString) {
        return MPAdTypeBanner;
    } else {
        return MPAdTypeUnknown;
    }
}

- (NSString *)networkTypeFromHeaders:(NSDictionary *)headers
{
    NSString *adTypeString = [headers objectForKey:kAdTypeHeaderKey];
    if ([adTypeString isEqualToString:@"interstitial"]) {
        return [headers objectForKey:kInterstitialAdTypeHeaderKey];
    } else {
        return adTypeString;
    }
}

- (NSURL *)URLFromHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSString *URLString = [headers objectForKey:key];
    return URLString ? [NSURL URLWithString:URLString] : nil;
}

- (NSDictionary *)dictionaryFromHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSData *data = [(NSString *)[headers objectForKey:key] dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *JSONFromHeaders = nil;
    if (data) {
        JSONFromHeaders = [NSJSONSerialization mp_JSONObjectWithData:data options:NSJSONReadingMutableContainers clearNullObjects:YES error:nil];
    }
    return JSONFromHeaders;
}

- (NSTimeInterval)refreshIntervalFromHeaders:(NSDictionary *)headers
{
    NSString *intervalString = [headers objectForKey:kRefreshTimeHeaderKey];
    NSTimeInterval interval = -1;
    if (intervalString) {
        interval = [intervalString doubleValue];
        if (interval < MINIMUM_REFRESH_INTERVAL) {
            interval = MINIMUM_REFRESH_INTERVAL;
        }
    }
    return interval;
}

- (NSTimeInterval)timeIntervalFromHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSString *intervalString = [headers objectForKey:key];
    NSTimeInterval interval = -1;
    if (intervalString) {
        int parsedInt = -1;
        BOOL isNumber = [[NSScanner scannerWithString:intervalString] scanInt:&parsedInt];
        if (isNumber && parsedInt >= 0) {
            interval = parsedInt;
        }
    }

    return interval;
}

- (NSTimeInterval)timeIntervalFromMsHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSString *msString = [headers objectForKey:key];
    NSTimeInterval interval = -1;
    if (msString) {
        int parsedInt = -1;
        BOOL isNumber = [[NSScanner scannerWithString:msString] scanInt:&parsedInt];
        if (isNumber && parsedInt >= 0) {
            interval = parsedInt / 1000.0f;
        }
    }

    return interval;
}

- (NSInteger)percentFromHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSString *percentString = [headers objectForKey:key];
    NSInteger percent = -1;
    if (percentString) {
        int parsedInt = -1;
        BOOL isNumber = [[NSScanner scannerWithString:percentString] scanInt:&parsedInt];
        if (isNumber && parsedInt >= 0 && parsedInt <= 100) {
            percent = parsedInt;
        }
    }

    return percent;
}

- (NSNumber *)adAmountFromHeaders:(NSDictionary *)headers key:(NSString *)key
{
    NSString *amountString = [headers objectForKey:key];
    NSNumber *amount = @(-1);
    if (amountString) {
        int parsedInt = -1;
        BOOL isNumber = [[NSScanner scannerWithString:amountString] scanInt:&parsedInt];
        if (isNumber && parsedInt >= 0) {
            amount = @(parsedInt);
        }
    }

    return amount;
}

- (MPInterstitialOrientationType)orientationTypeFromHeaders:(NSDictionary *)headers
{
    NSString *orientation = [headers objectForKey:kOrientationTypeHeaderKey];
    if ([orientation isEqualToString:@"p"]) {
        return MPInterstitialOrientationTypePortrait;
    } else if ([orientation isEqualToString:@"l"]) {
        return MPInterstitialOrientationTypeLandscape;
    } else {
        return MPInterstitialOrientationTypeAll;
    }
}

#if MP_HAS_NATIVE_PACKAGE
- (NSDictionary *)nativeVideoTrackersFromHeaders:(NSDictionary *)headers key:(NSString *)key
{
    NSDictionary *dictFromHeader = [self dictionaryFromHeaders:headers forKey:key];
    if (!dictFromHeader) {
        return nil;
    }
    NSMutableDictionary *videoTrackerDict = [NSMutableDictionary new];
    NSArray *events = dictFromHeader[kNativeVideoTrackerEventsHeaderKey];
    NSArray *urls = dictFromHeader[kNativeVideoTrackerUrlsHeaderKey];
    NSSet *supportedEvents = [NSSet setWithObjects:MPVASTTrackingEventTypeStart, MPVASTTrackingEventTypeFirstQuartile, MPVASTTrackingEventTypeMidpoint,  MPVASTTrackingEventTypeThirdQuartile, MPVASTTrackingEventTypeComplete, nil];
    for (NSString *event in events) {
        if (![supportedEvents containsObject:event]) {
            continue;
        }
        [self setVideoTrackers:videoTrackerDict event:event urls:urls];
    }
    if (videoTrackerDict.count == 0) {
        return nil;
    }
    return videoTrackerDict;
}

- (void)setVideoTrackers:(NSMutableDictionary *)videoTrackerDict event:(NSString *)event urls:(NSArray *)urls {
    NSMutableArray *trackers = [NSMutableArray new];
    for (NSString *url in urls) {
        if ([url rangeOfString:kNativeVideoTrackerUrlMacro].location != NSNotFound) {
            NSString *trackerUrl = [url stringByReplacingOccurrencesOfString:kNativeVideoTrackerUrlMacro withString:event];
            NSDictionary *dict = @{kNativeVideoTrackerEventDictionaryKey:event, kNativeVideoTrackerTextDictionaryKey:trackerUrl};
            MPVASTTrackingEvent *tracker = [[MPVASTTrackingEvent alloc] initWithDictionary:dict];
            [trackers addObject:tracker];
        }
    }
    if (trackers.count > 0) {
        videoTrackerDict[event] = trackers;
    }
}

#endif

- (NSArray *)parseAvailableRewardsFromHeaders:(NSDictionary *)headers {
    // The X-Rewarded-Currencies header key doesn't exist. This is probably
    // not a rewarded ad.
    NSDictionary * currencies = [self dictionaryFromHeaders:headers forKey:kRewardedCurrenciesHeaderKey];
    if (currencies == nil) {
        return nil;
    }

    // Either the list of available rewards doesn't exist or is empty.
    // This is an error.
    NSArray * rewards = [currencies objectForKey:@"rewards"];
    if (rewards.count == 0) {
        MPLogError(@"No available rewards found.");
        return nil;
    }

    // Parse the list of JSON rewards into objects.
    NSMutableArray * availableRewards = [NSMutableArray arrayWithCapacity:rewards.count];
    [rewards enumerateObjectsUsingBlock:^(NSDictionary * rewardDict, NSUInteger idx, BOOL * _Nonnull stop) {
        NSString * name = rewardDict[@"name"] ?: kMPRewardedVideoRewardCurrencyTypeUnspecified;
        NSNumber * amount = rewardDict[@"amount"] ?: @(kMPRewardedVideoRewardCurrencyAmountUnspecified);

        MPRewardedVideoReward * reward = [[MPRewardedVideoReward alloc] initWithCurrencyType:name amount:amount];
        [availableRewards addObject:reward];
    }];

    return availableRewards;
}

- (MOPUBDisplayAgentType)clickthroughExperimentVariantFromHeaders:(NSDictionary *)headers forKey:(NSString *)key
{
    NSString *variantString = [headers objectForKey:key];
    NSInteger variant = 0;
    if (variantString) {
        int parsedInt = -1;
        BOOL isNumber = [[NSScanner scannerWithString:variantString] scanInt:&parsedInt];
        if (isNumber && parsedInt >= 0 && parsedInt <= kMaximumVariantForClickthroughExperiment) {
            variant = parsedInt;
        }
    }

    return variant;
}

- (BOOL)visibleImpressionTrackingEnabled
{
    if (self.impressionMinVisibleTimeInSec < 0 || self.impressionMinVisiblePixels <= 0) {
        return NO;
    }
    return YES;
}

@end
