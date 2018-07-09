//
//  MOPUBNativeVideoAdAdapter.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBNativeVideoAdAdapter.h"
#import "MPNativeAdError.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPNativeAdConstants.h"
#import "MPLogging.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MPAdImpressionTimer.h"

@interface MOPUBNativeVideoAdAdapter() <MPAdDestinationDisplayAgentDelegate, MPAdImpressionTimerDelegate>

@property (nonatomic) MPAdImpressionTimer *impressionTimer;
@property (nonatomic, readonly) MPAdDestinationDisplayAgent *destinationDisplayAgent;

@end

@implementation MOPUBNativeVideoAdAdapter

@synthesize properties = _properties;
@synthesize defaultActionURL = _defaultActionURL;

- (instancetype)initWithAdProperties:(NSMutableDictionary *)properties
{
    if (self = [super init]) {

        // Let's make sure the data types of all the provided native ad properties are strings before creating the adapter.

        NSArray *keysToCheck = @[kAdIconImageKey, kAdMainImageKey, kAdTextKey, kAdTitleKey, kAdCTATextKey, kVASTVideoKey];

        for (NSString *key in keysToCheck) {
            id value = properties[key];
            if (value != nil && ![value isKindOfClass:[NSString class]]) {
                return nil;
            }
        }

        BOOL valid = YES;
        NSArray *impressionTrackers = [properties objectForKey:kImpressionTrackerURLsKey];
        if (![impressionTrackers isKindOfClass:[NSArray class]] || [impressionTrackers count] < 1) {
            valid = NO;
        } else {
            _impressionTrackerURLs = MPConvertStringArrayToURLArray(impressionTrackers);
        }

        NSObject *clickTracker = [properties objectForKey:kClickTrackerURLKey];

        // The click tracker could either be a single URL or an array of URLS.
        if ([clickTracker isKindOfClass:[NSArray class]]) {
            _clickTrackerURLs = MPConvertStringArrayToURLArray((NSArray *)clickTracker);
        } else if ([clickTracker isKindOfClass:[NSString class]]) {
            NSURL *url = [NSURL URLWithString:(NSString *)clickTracker];
            if (url) {
                _clickTrackerURLs = @[ url ];
            } else {
                valid = NO;
            }
        } else {
            valid = NO;
        }

        _defaultActionURL = [NSURL URLWithString:[properties objectForKey:kDefaultActionURLKey]];

        [properties removeObjectsForKeys:[NSArray arrayWithObjects:kImpressionTrackerURLsKey, kClickTrackerURLKey, kDefaultActionURLKey, nil]];
        _properties = properties;

        if (!valid) {
            return nil;
        }

        // Add the DAA icon settings to our properties dictionary.
        [properties setObject:MPResourcePathForResource(kDAAIconImageName) forKey:kAdDAAIconImageKey];

        _destinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];

        _impressionTimer = nil;
    }

    return self;
}

- (void)dealloc
{
    [self removeStaticImpressionTimer];
    [_destinationDisplayAgent cancel];
    [_destinationDisplayAgent setDelegate:nil];
    _delegate = nil;
}

#pragma mark - Private

- (void)removeStaticImpressionTimer
{
    _impressionTimer.delegate = nil;
    _impressionTimer = nil;
}

#pragma mark - <MPNativeAdAdapter>

- (void)willAttachToView:(UIView *)view
{
    [self removeStaticImpressionTimer];

    // Set up an impression timer that will fire the mopub impression if the video fails to play prior to meeting the video impression tracking requirements.
    MOPUBNativeVideoAdConfigValues *nativeVideoAdConfig = [self.properties objectForKey:kNativeAdConfigKey];

    // If we have a valid pixel value, use it to track the impression. If not, use percentage instead.
    if (nativeVideoAdConfig.isImpressionMinVisiblePixelsValid) {
        self.impressionTimer = [[MPAdImpressionTimer alloc] initWithRequiredSecondsForImpression:nativeVideoAdConfig.impressionMinVisibleSeconds
                                                                    requiredViewVisibilityPixels:nativeVideoAdConfig.impressionMinVisiblePixels];
    } else {
        // impressionMinVisiblePercent is an integer (a value of 50 means 50%) while the impression timer takes in a float (.50 means 50%) so we have to multiply it by .01f.
        self.impressionTimer = [[MPAdImpressionTimer alloc] initWithRequiredSecondsForImpression:nativeVideoAdConfig.impressionMinVisibleSeconds
                                                                requiredViewVisibilityPercentage:nativeVideoAdConfig.impressionMinVisiblePercent * 0.01f];
    }
    self.impressionTimer.delegate = self;

    [self.impressionTimer startTrackingView:view];
}

- (void)displayContentForURL:(NSURL *)URL rootViewController:(UIViewController *)controller
{
    if (!controller) {
        return;
    }

    if (!URL || ![URL isKindOfClass:[NSURL class]] || ![URL.absoluteString length]) {
        return;
    }

    [self.destinationDisplayAgent displayDestinationForURL:URL];
}

#pragma mark - DAA Icon

- (void)displayContentForDAAIconTap
{
    [self.destinationDisplayAgent displayDestinationForURL:[NSURL URLWithString:kDAAIconTapDestinationURL]];
}

#pragma mark - Impression and click tracking. Renderer calls those two methods

- (void)handleVideoViewImpression
{
    [self.delegate nativeAdWillLogImpression:self];
}

- (void)handleVideoViewClick
{
    [self.delegate nativeAdDidClick:self];
}

- (void)handleVideoHasProgressedToTime:(NSTimeInterval)playbackTime
{
    // If the video makes progress, don't allow static impression tracking.
    self.impressionTimer.delegate = nil;
    self.impressionTimer = nil;
}

#pragma mark - <MPAdImpressionTimerDelegate>

- (void)adViewWillLogImpression:(UIView *)adView
{
    // We'll fire a static impression if the video hasn't started playing by the time the static impression timer has met its requirements.
    [self.delegate nativeAdWillLogImpression:self];
}

#pragma mark - <MPAdDestinationDisplayAgentDelegate>

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)displayAgentWillPresentModal
{
    [self.delegate nativeAdWillPresentModalForAdapter:self];
}

- (void)displayAgentWillLeaveApplication
{
    [self.delegate nativeAdWillLeaveApplicationFromAdapter:self];
}

- (void)displayAgentDidDismissModal
{
    [self.delegate nativeAdDidDismissModalForAdapter:self];
}

// -adConfiguration delegate method is automatically implemented via the adConfiguration property declaration.
@end
