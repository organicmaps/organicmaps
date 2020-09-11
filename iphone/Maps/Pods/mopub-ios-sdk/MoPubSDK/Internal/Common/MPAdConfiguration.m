//
//  MPAdConfiguration.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBExperimentProvider.h"
#import "MPAdConfiguration.h"
#import "MPAdServerKeys.h"
#import "MPConstants.h"
#import "MPHTMLBannerCustomEvent.h"
#import "MPHTMLInterstitialCustomEvent.h"
#import "MPLogging.h"
#import "MPMoPubRewardedPlayableCustomEvent.h"
#import "MPMoPubRewardedVideoCustomEvent.h"
#import "MPMRAIDBannerCustomEvent.h"
#import "MPMRAIDInterstitialCustomEvent.h"
#import "MPRewardedVideoReward.h"
#import "MPVASTInterstitialCustomEvent.h"
#import "MPVASTTracking.h"
#import "MPViewabilityTracker.h"
#import "NSDictionary+MPAdditions.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "NSString+MPAdditions.h"

#if __has_include("MOPUBNativeVideoCustomEvent.h")
#import "MOPUBNativeVideoCustomEvent.h"
#endif

#if __has_include("MPMoPubNativeCustomEvent.h")
#import "MPMoPubNativeCustomEvent.h"
#endif

#if __has_include("MPVASTTrackingEvent.h")
#import "MPVASTTrackingEvent.h"
#endif

// MACROS
#define AFTER_LOAD_DURATION_MACRO   @"%%LOAD_DURATION_MS%%"
#define AFTER_LOAD_RESULT_MACRO   @"%%LOAD_RESULT%%"

typedef NS_ENUM(NSUInteger, MPVASTPlayerVersion) {
    MPVASTPlayerVersionUndetermined = 0, // default value, should be treated as web view player
    MPVASTPlayerVersionWebViewPlayer = 1,
    MPVASTPlayerVersionNativePlayer = 2
};

NSString * const kAdTypeMetadataKey = @"x-adtype";
NSString * const kAdUnitWarmingUpMetadataKey = @"x-warmup";
NSString * const kClickthroughMetadataKey = @"x-clickthrough";
NSString * const kCreativeIdMetadataKey = @"x-creativeid";
NSString * const kCustomEventClassNameMetadataKey = @"x-custom-event-class-name";
NSString * const kCustomEventClassDataMetadataKey = @"x-custom-event-class-data";
NSString * const kNextUrlMetadataKey = @"x-next-url";
NSString * const kFormatMetadataKey = @"adunit-format";
NSString * const kBeforeLoadUrlMetadataKey = @"x-before-load-url";
NSString * const kAfterLoadUrlMetadataKey = @"x-after-load-url";
NSString * const kAfterLoadSuccessUrlMetadataKey = @"x-after-load-success-url";
NSString * const kAfterLoadFailureUrlMetadataKey = @"x-after-load-fail-url";
NSString * const kHeightMetadataKey = @"x-height";
NSString * const kImpressionTrackerMetadataKey = @"x-imptracker"; // Deprecated; "imptrackers" if available
NSString * const kImpressionTrackersMetadataKey = @"imptrackers";
NSString * const kNativeSDKParametersMetadataKey = @"x-nativeparams";
NSString * const kNetworkTypeMetadataKey = @"x-networktype";
NSString * const kRefreshTimeMetadataKey = @"x-refreshtime";
NSString * const kAdTimeoutMetadataKey = @"x-ad-timeout-ms";
NSString * const kWidthMetadataKey = @"x-width";
NSString * const kDspCreativeIdKey = @"x-dspcreativeid";
NSString * const kPrecacheRequiredKey = @"x-precacherequired";
NSString * const kIsVastVideoPlayerKey = @"x-vastvideoplayer";
NSString * const kImpressionDataMetadataKey = @"impdata";

NSString * const kFullAdTypeMetadataKey = @"x-fulladtype";
NSString * const kOrientationTypeMetadataKey = @"x-orientation";

NSString * const kNativeImpressionMinVisiblePixelsMetadataKey = @"x-native-impression-min-px"; // The pixels Metadata takes priority over percentage, but percentage is left for backwards compatibility
NSString * const kNativeImpressionMinVisiblePercentMetadataKey = @"x-impression-min-visible-percent";
NSString * const kNativeImpressionVisibleMsMetadataKey = @"x-impression-visible-ms";
NSString * const kNativeVideoPlayVisiblePercentMetadataKey = @"x-play-visible-percent";
NSString * const kNativeVideoPauseVisiblePercentMetadataKey = @"x-pause-visible-percent";
NSString * const kNativeVideoMaxBufferingTimeMsMetadataKey = @"x-max-buffer-ms";
NSString * const kVASTVideoTrackersMetadataKey = @"x-video-trackers";

NSString * const kBannerImpressionVisableMsMetadataKey = @"x-banner-impression-min-ms";
NSString * const kBannerImpressionMinPixelMetadataKey = @"x-banner-impression-min-pixels";

NSString * const kAdTypeHtml = @"html";
NSString * const kAdTypeInterstitial = @"interstitial";
NSString * const kAdTypeMraid = @"mraid";
NSString * const kAdTypeClear = @"clear";
NSString * const kAdTypeNative = @"json";
NSString * const kAdTypeNativeVideo = @"json_video";
NSString * const kAdTypeRewardedVideo = @"rewarded_video";
NSString * const kAdTypeRewardedPlayable = @"rewarded_playable";
NSString * const kAdTypeVAST = @"vast"; // a possible value of "x-fulladtype"

// rewarded video
NSString * const kRewardedVideoCurrencyNameMetadataKey = @"x-rewarded-video-currency-name";
NSString * const kRewardedVideoCurrencyAmountMetadataKey = @"x-rewarded-video-currency-amount";
NSString * const kRewardedVideoCompletionUrlMetadataKey = @"x-rewarded-video-completion-url";
NSString * const kRewardedCurrenciesMetadataKey = @"x-rewarded-currencies";

// rewarded playables
NSString * const kRewardedPlayableDurationMetadataKey = @"x-rewarded-duration";
NSString * const kRewardedPlayableRewardOnClickMetadataKey = @"x-should-reward-on-click";

// vast video trackers
NSString * const kVASTVideoTrackerUrlMacro = @"%%VIDEO_EVENT%%";
NSString * const kVASTVideoTrackerEventsMetadataKey = @"events";
NSString * const kVASTVideoTrackerUrlsMetadataKey = @"urls";
NSString * const kVASTVideoTrackerEventDictionaryKey = @"event";
NSString * const kVASTVideoTrackerTextDictionaryKey = @"text";

// clickthrough experiment
NSString * const kClickthroughExperimentBrowserAgent = @"x-browser-agent";
static const NSInteger kMaximumVariantForClickthroughExperiment = 2;

// viewability
NSString * const kViewabilityDisableMetadataKey = @"x-disable-viewability";

// advanced bidding
NSString * const kAdvancedBiddingMarkupMetadataKey = @"adm";

// Correspond to a numeric value: 2 means native player, 1 or 0 means MoVideo web view player
NSString * const kVASTPlayerVersionKey = @"vast-player-version";

// MRAID
NSString * const kMRAIDAllowCustomCloseKey = @"allow-custom-close";

/**
 Format Unification Phase 2 item 1.1 - clickability experiment
 When the experiment is enabled, users are able to click a fullscreen non-rewarded VAST video ad
 immediately, so that they can consume additional content about the advertiser. Clicking on this
 video should launch the CTA.
 */
NSString * const kVASTClickabilityExperimentKey = @"vast-click-enabled";

@interface MPAdConfiguration ()

@property (nonatomic, copy) NSString *adResponseHTMLString;
@property (nonatomic, strong, readwrite) NSArray<MPRewardedVideoReward *> *availableRewards;
@property (nonatomic) MOPUBDisplayAgentType clickthroughExperimentBrowserAgent;
@property (nonatomic, strong) MOPUBExperimentProvider *experimentProvider;

@property (nonatomic, copy) NSArray <NSString *> *afterLoadUrlsWithMacros;
@property (nonatomic, copy) NSArray <NSString *> *afterLoadSuccessUrlsWithMacros;
@property (nonatomic, copy) NSArray <NSString *> *afterLoadFailureUrlsWithMacros;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdConfiguration

- (instancetype)initWithMetadata:(NSDictionary *)metadata data:(NSData *)data isFullscreenAd:(BOOL)isFullscreenAd
{
    self = [super init];
    if (self) {
        [self commonInitWithMetadata:metadata
                                data:data
                      isFullscreenAd:isFullscreenAd
                  experimentProvider:MOPUBExperimentProvider.sharedInstance];
    }
    return self;
}

/**
 This common init enables unit testing with an `MOPUBExperimentProvider` instance that is not a singleton.
 */
- (void)commonInitWithMetadata:(NSDictionary *)metadata
                          data:(NSData *)data
                isFullscreenAd:(BOOL)isFullscreenAd
            experimentProvider:(MOPUBExperimentProvider *)experimentProvider
{
    self.adResponseData = data;

    _isFullscreenAd = isFullscreenAd;
    self.adUnitWarmingUp = [metadata mp_boolForKey:kAdUnitWarmingUpMetadataKey];

    self.adType = [self adTypeFromMetadata:metadata];

    self.preferredSize = CGSizeMake([metadata mp_floatForKey:kWidthMetadataKey],
                                    [metadata mp_floatForKey:kHeightMetadataKey]);

    self.clickTrackingURL = [self URLFromMetadata:metadata
                                           forKey:kClickthroughMetadataKey];
    self.nextURL = [self URLFromMetadata:metadata
                                  forKey:kNextUrlMetadataKey];
    self.format = [metadata objectForKey:kFormatMetadataKey];
    self.beforeLoadURL = [self URLFromMetadata:metadata forKey:kBeforeLoadUrlMetadataKey];
    self.afterLoadUrlsWithMacros = [self URLStringsFromMetadata:metadata forKey:kAfterLoadUrlMetadataKey];
    self.afterLoadSuccessUrlsWithMacros = [self URLStringsFromMetadata:metadata forKey:kAfterLoadSuccessUrlMetadataKey];
    self.afterLoadFailureUrlsWithMacros = [self URLStringsFromMetadata:metadata forKey:kAfterLoadFailureUrlMetadataKey];

    self.refreshInterval = [self refreshIntervalFromMetadata:metadata];
    self.adTimeoutInterval = [self timeIntervalFromMsmetadata:metadata forKey:kAdTimeoutMetadataKey];

    self.nativeSDKParameters = [self dictionaryFromMetadata:metadata
                                                     forKey:kNativeSDKParametersMetadataKey];

    self.orientationType = [self orientationTypeFromMetadata:metadata];

    switch ([metadata mp_unsignedIntegerForKey:kVASTPlayerVersionKey]) {
        case MPVASTPlayerVersionNativePlayer:
            self.customEventClass = [self setUpCustomEventClassFromMetadata:metadata
                                                          vastPlayerVersion:MPVASTPlayerVersionNativePlayer];
            break;
        default:
            self.customEventClass = [self setUpCustomEventClassFromMetadata:metadata
                                                          vastPlayerVersion:MPVASTPlayerVersionWebViewPlayer];
            break;
    }

    self.customEventClassData = [self customEventClassDataFromMetadata:metadata];

    self.dspCreativeId = [metadata objectForKey:kDspCreativeIdKey];

    self.precacheRequired = [metadata mp_boolForKey:kPrecacheRequiredKey];

    self.isVastVideoPlayer = [metadata mp_boolForKey:kIsVastVideoPlayerKey];

    self.creationTimestamp = [NSDate date];

    self.creativeId = [metadata objectForKey:kCreativeIdMetadataKey];

    self.metadataAdType = [metadata objectForKey:kAdTypeMetadataKey];

    self.nativeVideoPlayVisiblePercent = [self percentFromMetadata:metadata forKey:kNativeVideoPlayVisiblePercentMetadataKey];

    self.nativeVideoPauseVisiblePercent = [self percentFromMetadata:metadata forKey:kNativeVideoPauseVisiblePercentMetadataKey];

    self.nativeImpressionMinVisiblePixels = [[self adAmountFromMetadata:metadata key:kNativeImpressionMinVisiblePixelsMetadataKey] floatValue];

    self.nativeImpressionMinVisiblePercent = [self percentFromMetadata:metadata forKey:kNativeImpressionMinVisiblePercentMetadataKey];

    self.nativeImpressionMinVisibleTimeInterval = [self timeIntervalFromMsmetadata:metadata forKey:kNativeImpressionVisibleMsMetadataKey];

    self.nativeVideoMaxBufferingTime = [self timeIntervalFromMsmetadata:metadata forKey:kNativeVideoMaxBufferingTimeMsMetadataKey];

    // VAST video trackers
    self.vastVideoTrackers = [self vastVideoTrackersFromMetadata:metadata key:kVASTVideoTrackersMetadataKey];

    self.impressionMinVisibleTimeInSec = [self timeIntervalFromMsmetadata:metadata forKey:kBannerImpressionVisableMsMetadataKey];
    self.impressionMinVisiblePixels = [[self adAmountFromMetadata:metadata key:kBannerImpressionMinPixelMetadataKey] floatValue];

    self.impressionData = [self impressionDataFromMetadata:metadata];
    self.enableEarlyClickthroughForNonRewardedVideo = [metadata mp_boolForKey:kVASTClickabilityExperimentKey defaultValue:NO];

    // Organize impression tracking URLs
    NSArray <NSURL *> * URLs = [self URLsFromMetadata:metadata forKey:kImpressionTrackersMetadataKey];
    // Check to see if the array actually contains URLs
    if (URLs.count > 0) {
        self.impressionTrackingURLs = URLs;
    } else {
        // If the array does not contain URLs, take the old `x-imptracker` URL and save that into an array instead.
        self.impressionTrackingURLs = [self URLsFromMetadata:metadata forKey:kImpressionTrackerMetadataKey];
    }

    // rewarded video

    // Attempt to parse the multiple currency Metadata first since this will take
    // precedence over the older single currency approach.
    self.availableRewards = [self parseAvailableRewardsFromMetadata:metadata];
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
    // metadata.
    else {
        NSString *currencyName = [metadata objectForKey:kRewardedVideoCurrencyNameMetadataKey] ?: kMPRewardedVideoRewardCurrencyTypeUnspecified;

        NSNumber *currencyAmount = [self adAmountFromMetadata:metadata key:kRewardedVideoCurrencyAmountMetadataKey];
        if (currencyAmount.integerValue <= 0) {
            currencyAmount = @(kMPRewardedVideoRewardCurrencyAmountUnspecified);
        }

        MPRewardedVideoReward * reward = [[MPRewardedVideoReward alloc] initWithCurrencyType:currencyName amount:currencyAmount];
        self.availableRewards = [NSArray arrayWithObject:reward];
        self.selectedReward = reward;
    }

    self.rewardedVideoCompletionUrl = [metadata objectForKey:kRewardedVideoCompletionUrlMetadataKey];

    // rewarded playables
    self.rewardedPlayableDuration = [self timeIntervalFromMetadata:metadata forKey:kRewardedPlayableDurationMetadataKey];
    self.rewardedPlayableShouldRewardOnClick = [[metadata objectForKey:kRewardedPlayableRewardOnClickMetadataKey] boolValue];

    // clickthrough experiment
    self.clickthroughExperimentBrowserAgent = [self clickthroughExperimentVariantFromMetadata:metadata forKey:kClickthroughExperimentBrowserAgent];
    self.experimentProvider = experimentProvider;
    [self.experimentProvider setDisplayAgentFromAdServer:self.clickthroughExperimentBrowserAgent];

    // viewability
    NSInteger disabledViewabilityValue = [metadata mp_integerForKey:kViewabilityDisableMetadataKey];

    if (disabledViewabilityValue != 0 &&
        disabledViewabilityValue >= MPViewabilityOptionNone &&
        disabledViewabilityValue <= MPViewabilityOptionAll) {
        MPViewabilityOption vendorsToDisable = (MPViewabilityOption)disabledViewabilityValue;
        [MPViewabilityTracker disableViewability:vendorsToDisable];
    }

    // advanced bidding
    self.advancedBidPayload = [metadata objectForKey:kAdvancedBiddingMarkupMetadataKey];

    // MRAID
    _mraidAllowCustomClose = [metadata mp_boolForKey:kMRAIDAllowCustomCloseKey defaultValue:NO];
}

/**
 Provided the metadata of an ad, return the class of corresponding custome event.
 */
- (Class)setUpCustomEventClassFromMetadata:(NSDictionary *)metadata
                         vastPlayerVersion:(MPVASTPlayerVersion)vastPlayerVersion
{
    NSDictionary *customEventTable;
    if (self.isFullscreenAd) {
        Class rewardedVideoClass;
        switch (vastPlayerVersion) {
            case MPVASTPlayerVersionNativePlayer:
                rewardedVideoClass = [MPVASTInterstitialCustomEvent class];
                break;
            default: // web view player
                rewardedVideoClass = [MPMoPubRewardedVideoCustomEvent class];
                break;
        }

        customEventTable = @{@"admob_full": @"MPGoogleAdMobInterstitialCustomEvent", // optional class
        kAdTypeHtml: NSStringFromClass([MPHTMLInterstitialCustomEvent class]),
        kAdTypeMraid: NSStringFromClass([MPMRAIDInterstitialCustomEvent class]),
        kAdTypeRewardedVideo: NSStringFromClass(rewardedVideoClass),
        kAdTypeRewardedPlayable: NSStringFromClass([MPMoPubRewardedPlayableCustomEvent class]),
        kAdTypeVAST: NSStringFromClass([MPVASTInterstitialCustomEvent class])};
    } else {
        customEventTable = @{@"admob_native": @"MPGoogleAdMobBannerCustomEvent", // optional class
        kAdTypeHtml: NSStringFromClass([MPHTMLBannerCustomEvent class]),
        kAdTypeMraid: NSStringFromClass([MPMRAIDBannerCustomEvent class]),
        kAdTypeNativeVideo: @"MOPUBNativeVideoCustomEvent", // optional native class
        kAdTypeNative: @"MPMoPubNativeCustomEvent"};        // optional native class
    }

    NSString *customEventClassName = metadata[kCustomEventClassNameMetadataKey];
    if (customEventTable[self.adType]) {
        customEventClassName = customEventTable[self.adType];
    }

    Class customEventClass = NSClassFromString(customEventClassName);
    if (customEventClassName && !customEventClass) {
        MPLogInfo(@"Could not find custom event class named %@", customEventClassName);
    }

    return customEventClass;
}

- (NSDictionary *)customEventClassDataFromMetadata:(NSDictionary *)metadata
{
    // Parse out custom event data if its present
    NSDictionary *result = [self dictionaryFromMetadata:metadata forKey:kCustomEventClassDataMetadataKey];
    if (result != nil) {
        // Inject the unified ad unit format into the custom data so that
        // all adapters (including mediated ones) can differentiate between
        // banner and medium rectangle formats.
        // The key `adunit_format` is used to denote the format, which is the same as the
        // key for impression level revenue data since they represent the same information.
        NSString *format = [metadata objectForKey:kFormatMetadataKey];
        if (format.length > 0) {
            NSMutableDictionary *dictionary = [result mutableCopy];
            dictionary[kImpressionDataAdUnitFormatKey] = format;
            result = dictionary;
        }
    }
    // No custom event data found; this is probably a native ad payload.
    else {
        result = [self dictionaryFromMetadata:metadata forKey:kNativeSDKParametersMetadataKey];
    }
    return result;
}


- (BOOL)hasPreferredSize
{
    return (self.preferredSize.width > 0 && self.preferredSize.height > 0);
}

- (BOOL)hasValidReward
{
    return (self.availableRewards.firstObject != nil
            && [self.availableRewards.firstObject.currencyType isEqualToString:kMPRewardedVideoRewardCurrencyTypeUnspecified] == NO);
}

- (NSString *)adResponseHTMLString
{
    if (!_adResponseHTMLString) {
        self.adResponseHTMLString = [[NSString alloc] initWithData:self.adResponseData
                                                           encoding:NSUTF8StringEncoding];
    }

    return _adResponseHTMLString;
}

- (NSArray <NSURL *> *)afterLoadUrlsWithLoadDuration:(NSTimeInterval)duration loadResult:(MPAfterLoadResult)result
{
    NSArray <NSString *> * afterLoadUrls = [self concatenateBaseUrlArray:self.afterLoadUrlsWithMacros
                                                    withConditionalArray:(result == MPAfterLoadResultAdLoaded ? self.afterLoadSuccessUrlsWithMacros : self.afterLoadFailureUrlsWithMacros)];

    // No URLs to generate
    if (afterLoadUrls == nil || afterLoadUrls.count == 0) {
        return nil;
    }

    NSMutableArray * urls = [NSMutableArray arrayWithCapacity:afterLoadUrls.count];

    for (NSString * urlString in afterLoadUrls) {
        // Skip if the URL length is 0
        if (urlString.length == 0) {
            continue;
        }

        // Generate the ad server value from the enumeration. If the result type failed to
        // match, we should not process this any further.
        NSString * resultString = nil;
        switch (result) {
            case MPAfterLoadResultError: resultString = @"error"; break;
            case MPAfterLoadResultTimeout: resultString = @"timeout"; break;
            case MPAfterLoadResultAdLoaded: resultString = @"ad_loaded"; break;
            case MPAfterLoadResultMissingAdapter: resultString = @"missing_adapter"; break;
            default: return nil;
        }

        // Convert the duration to milliseconds
        NSString * durationMs = [NSString stringWithFormat:@"%llu", (unsigned long long)(duration * 1000)];

        // Replace the macros
        NSString * expandedUrl = [urlString stringByReplacingOccurrencesOfString:AFTER_LOAD_DURATION_MACRO withString:durationMs];
        expandedUrl = [expandedUrl stringByReplacingOccurrencesOfString:AFTER_LOAD_RESULT_MACRO withString:resultString];

        // Add to array (@c URLWithString may return @c nil, so check before appending to the array)
        NSURL * url = [NSURL URLWithString:expandedUrl];
        if (url != nil) {
            [urls addObject:url];
        }
    }

    return urls.count > 0 ? urls : nil;
}

- (BOOL)isMraidAd
{
    return [self.metadataAdType isEqualToString:kAdTypeMraid];
}

- (BOOL)isMoVideo
{
    // Comparing using class name strings instead of `isKindOfClass:`. The isKindOfClass:`
    // check fails because `self.customEventClass` was instantiated using reflection.
    NSString *className = NSStringFromClass(self.customEventClass);
    BOOL isMoVideoRewardedVideo = [className isEqualToString:@"MPMoPubRewardedVideoCustomEvent"];
    BOOL isMoVideoRewardedPlayable = [className isEqualToString:@"MPMoPubRewardedPlayableCustomEvent"];
    return isMoVideoRewardedVideo || isMoVideoRewardedPlayable;
}

#pragma mark - Private

- (NSArray *)concatenateBaseUrlArray:(NSArray *)baseArray withConditionalArray:(NSArray *)conditionalArray {
    if (baseArray == nil && conditionalArray == nil) {
        return nil;
    }

    if (baseArray == nil) {
        return conditionalArray;
    }

    if (conditionalArray == nil) {
        return baseArray;
    }

    return [baseArray arrayByAddingObjectsFromArray:conditionalArray];
}

/**
 Read the ad type from the "x-adtype" and "x-fulladtype" of the provided @c metadata. The return
 value is non-null because ad type might be used as a dictionary key, and a nil key causes crash.
 @param metadata the dictionary that contains ad type information
 @return A non-null @c NSString. If @c metadata does not contain valid ad type value, then return
 an empty string.
*/
- (NSString * _Nonnull)adTypeFromMetadata:(NSDictionary *)metadata
{
    NSString *adTypeString = [metadata objectForKey:kAdTypeMetadataKey];

    // override ad type if full ad type is provided
    if ([adTypeString isEqualToString:kAdTypeInterstitial]
        && [[metadata objectForKey:kFullAdTypeMetadataKey] isKindOfClass:[NSString class]]
        && ((NSString *)[metadata objectForKey:kFullAdTypeMetadataKey]).length > 0) {
        adTypeString = [metadata objectForKey:kFullAdTypeMetadataKey];
    }

    // make sure the return value is non-null
    if (adTypeString.length == 0) {
        adTypeString = @"";
    }

    return adTypeString;
}

- (NSURL *)URLFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    NSString *URLString = [metadata objectForKey:key];
    return URLString ? [NSURL URLWithString:URLString] : nil;
}

/**
 Reads the value at key @c key from dictionary @c metadata. If the value is a @c NSString that is convertable to
 @c NSURL, it will be converted into an @c NSURL, inserted into an array, and returned. If the value is a @c NSArray,
 each @c NSString in the array that is convertable to @c NSURL will be converted and all returned in an array, with all
 other objects scrubbed. If the value from @c metadata is @c nil, not an @c NSString, not an @c NSArray, an @c NSString
 that cannot be converted to @c NSURL, or an @c NSArray that does not contain NSURL-convertable strings, this method
 will return @c nil.
 @remark This method converts all @c NSStrings into @c NSURLs, where possible. If this behavior is not desired,
 use @c URLStringsFromMetadata:forkey: instead.
 @param metadata the @c NSDictionary to read from
 @param key the @c the key to look up in @c metadata
 @return @c NSArray of @c NSURL contained at key @c key, or @c nil
 */
- (NSArray <NSURL *> *)URLsFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key {
    NSArray <NSString *> * URLStrings = [self URLStringsFromMetadata:metadata forKey:key];
    if (URLStrings == nil) {
        return nil;
    }

    // Convert the strings into NSURLs and save in a new array
    NSMutableArray <NSURL *> * URLs = [NSMutableArray arrayWithCapacity:URLStrings.count];
    for (NSString * URLString in URLStrings) {
        // @c URLWithString may return @c nil, so check before appending to the array
        NSURL * URL = [NSURL URLWithString:URLString];
        if (URL != nil) {
            [URLs addObject:URL];
        }
    }

    return URLs.count > 0 ? URLs : nil;
}

/**
 Reads the value at key @c key from dictionary @c metadata. If the value is a @c NSString, it will be inserted into
 an array and returned. If the value is a @c NSArray, the @c NSStrings contained in that array will be all be returned
 in an array, with any object that is not an @c NSString scrubbed. If the value from @c metadata is @c nil, not an
 @c NSString, not an @c NSArray, or an @c NSArray that does not contain strings, this method will return @c nil.
 @remark This method does not convert the @c NSStrings into @c NSURLs. Use @c URLsFromMetadata:forKey: for that instead.
 @param metadata the @c NSDictionary to read from
 @param key the @c the key to look up in @c metadata
 @return @c NSArray of @c NSStrings contained at key @c key, or @c nil
 */
- (NSArray <NSString *> *)URLStringsFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key {
    NSObject * value = metadata[key];

    if (value == nil) {
        return nil;
    }

    if ([value isKindOfClass:[NSString class]]) {
        NSString * string = (NSString *)value;
        return string.length > 0 ? @[string] : nil;
    }

    if ([value isKindOfClass:[NSArray class]]) {
        NSArray * objects = (NSArray *)value;
        NSMutableArray <NSString *> * URLStrings = [NSMutableArray arrayWithCapacity:objects.count];
        for (NSObject * object in objects) {
            if ([object isKindOfClass:[NSString class]]) {
                [URLStrings addObject:(NSString *)object];
            }
        }
        return URLStrings.count > 0 ? URLStrings : nil;
    }

    return nil;
}

- (NSDictionary *)dictionaryFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    NSData *data = [(NSString *)[metadata objectForKey:key] dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *JSONFromMetadata = nil;
    if (data) {
        JSONFromMetadata = [NSJSONSerialization mp_JSONObjectWithData:data options:NSJSONReadingMutableContainers clearNullObjects:YES error:nil];
    }
    return JSONFromMetadata;
}

- (NSTimeInterval)refreshIntervalFromMetadata:(NSDictionary *)metadata
{
    NSTimeInterval interval = [metadata mp_doubleForKey:kRefreshTimeMetadataKey defaultValue:MINIMUM_REFRESH_INTERVAL];
    if (interval < MINIMUM_REFRESH_INTERVAL) {
        interval = MINIMUM_REFRESH_INTERVAL;
    }
    return interval;
}

- (NSTimeInterval)timeIntervalFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    NSTimeInterval interval = [metadata mp_doubleForKey:key defaultValue:-1];
    return interval;
}

- (NSTimeInterval)timeIntervalFromMsmetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    NSTimeInterval interval = [metadata mp_doubleForKey:key defaultValue:-1];
    if (interval >= 0) {
        interval /= 1000.0f;
    }
    return interval;
}

- (NSInteger)percentFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    return [metadata mp_integerForKey:key defaultValue:-1];

}

- (NSNumber *)adAmountFromMetadata:(NSDictionary *)metadata key:(NSString *)key
{
    NSInteger amount = [metadata mp_integerForKey:key defaultValue:-1];
    return @(amount);
}

- (MPInterstitialOrientationType)orientationTypeFromMetadata:(NSDictionary *)metadata
{
    NSString *orientation = [metadata objectForKey:kOrientationTypeMetadataKey];
    if ([orientation isEqualToString:@"p"]) {
        return MPInterstitialOrientationTypePortrait;
    } else if ([orientation isEqualToString:@"l"]) {
        return MPInterstitialOrientationTypeLandscape;
    } else {
        return MPInterstitialOrientationTypeAll;
    }
}

- (NSDictionary<MPVideoEvent, NSArray<MPVASTTrackingEvent *> *> *)vastVideoTrackersFromMetadata:(NSDictionary *)metadata
                                                                                            key:(NSString *)key
{
    NSDictionary *dictFromMetadata = [self dictionaryFromMetadata:metadata forKey:key];
    if (!dictFromMetadata) {
        return nil;
    }

    NSMutableDictionary<MPVideoEvent, NSArray<MPVASTTrackingEvent *> *> *videoTrackerDict = [NSMutableDictionary new];
    NSArray<NSString *> *events = dictFromMetadata[kVASTVideoTrackerEventsMetadataKey];
    NSArray<NSString *> *urls = dictFromMetadata[kVASTVideoTrackerUrlsMetadataKey];

    for (MPVideoEvent event in events) {
        if (![MPVideoEvents isSupportedEvent:event]) {
            continue;
        }
        [self setVideoTrackers:videoTrackerDict event:event urls:urls];
    }
    if (videoTrackerDict.count == 0) {
        return nil;
    }
    return videoTrackerDict;
}

- (void)setVideoTrackers:(NSMutableDictionary<MPVideoEvent, NSArray<MPVASTTrackingEvent *> *> *)videoTrackerDict
                   event:(MPVideoEvent)event
                    urls:(NSArray<NSString *> *)urls {
    NSMutableArray<MPVASTTrackingEvent *> *trackers = [NSMutableArray new];
    [urls enumerateObjectsUsingBlock:^(NSString * _Nonnull urlString, NSUInteger idx, BOOL * _Nonnull stop) {
        // Perform macro replacement
        if ([urlString rangeOfString:kVASTVideoTrackerUrlMacro].location != NSNotFound) {
            NSString *trackerUrl = [urlString stringByReplacingOccurrencesOfString:kVASTVideoTrackerUrlMacro withString:event];
            NSDictionary *dict = @{kVASTVideoTrackerEventDictionaryKey:event, kVASTVideoTrackerTextDictionaryKey:trackerUrl};
            MPVASTTrackingEvent *tracker = [[MPVASTTrackingEvent alloc] initWithDictionary:dict];
            [trackers addObject:tracker];
        }
    }];

    // Add to events dictionary if there are trackers available
    if (trackers.count > 0) {
        videoTrackerDict[event] = trackers;
    }
}

- (NSArray<MPRewardedVideoReward *> *)parseAvailableRewardsFromMetadata:(NSDictionary *)metadata {
    // The X-Rewarded-Currencies Metadata key doesn't exist. This is probably
    // not a rewarded ad.
    NSDictionary * currencies = [metadata objectForKey:kRewardedCurrenciesMetadataKey];
    if (currencies == nil) {
        return nil;
    }

    // Either the list of available rewards doesn't exist or is empty.
    // This is an error.
    NSArray * rewards = [currencies objectForKey:@"rewards"];
    if (rewards.count == 0) {
        MPLogDebug(@"No available rewards found.");
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

- (MOPUBDisplayAgentType)clickthroughExperimentVariantFromMetadata:(NSDictionary *)metadata forKey:(NSString *)key
{
    NSInteger variant = [metadata mp_integerForKey:key];
    if (variant > kMaximumVariantForClickthroughExperiment) {
        variant = -1;
    }

    return variant;
}

- (MPImpressionData *)impressionDataFromMetadata:(NSDictionary *)metadata
{
    NSDictionary * impressionDataDictionary = metadata[kImpressionDataMetadataKey];
    if (impressionDataDictionary == nil) {
        return nil;
    }

    MPImpressionData * impressionData = [[MPImpressionData alloc] initWithDictionary:impressionDataDictionary];
    return impressionData;
}

@end
