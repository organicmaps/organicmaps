//
//  MPMoPubNativeAdAdapter.m
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPMoPubNativeAdAdapter.h"
#import "MPNativeAdError.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPNativeAdConstants.h"
#import "MPGlobal.h"
#import "MPNativeAdConfigValues.h"
#import "MPAdImpressionTimer.h"

static const NSTimeInterval kMoPubRequiredSecondsForImpression = 1.0;
static const CGFloat kMoPubRequiredViewVisibilityPercentage = 0.5;

@interface MPMoPubNativeAdAdapter () <MPAdDestinationDisplayAgentDelegate, MPAdImpressionTimerDelegate>

@property (nonatomic, strong) MPAdImpressionTimer *impressionTimer;
@property (nonatomic, readonly) MPAdDestinationDisplayAgent *destinationDisplayAgent;

@end

@implementation MPMoPubNativeAdAdapter

@synthesize properties = _properties;
@synthesize defaultActionURL = _defaultActionURL;

- (instancetype)initWithAdProperties:(NSMutableDictionary *)properties
{
    if (self = [super init]) {

        // Let's make sure the data types of all the provided native ad properties are strings before creating the adapter

        NSArray *keysToCheck = @[kAdIconImageKey, kAdMainImageKey, kAdTextKey, kAdTitleKey, kAdCTATextKey];

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
        
        // Grab the config, figure out requiredSecondsForImpression and requiredViewVisibilityPercentage,
        // and set up the timer.
        MPNativeAdConfigValues *config = properties[kNativeAdConfigKey];
        NSTimeInterval requiredSecondsForImpression = config.isImpressionMinVisibleSecondsValid ? config.impressionMinVisibleSeconds : kMoPubRequiredSecondsForImpression;
        if (config.isImpressionMinVisiblePixelsValid) {
            _impressionTimer = [[MPAdImpressionTimer alloc] initWithRequiredSecondsForImpression:requiredSecondsForImpression
                                                                          requiredViewVisibilityPixels:config.impressionMinVisiblePixels];
        } else {
            CGFloat requiredViewVisibilityPercentage = config.isImpressionMinVisiblePercentValid ? (config.impressionMinVisiblePercent / 100.0) : kMoPubRequiredViewVisibilityPercentage;
            _impressionTimer = [[MPAdImpressionTimer alloc] initWithRequiredSecondsForImpression:requiredSecondsForImpression
                                                                      requiredViewVisibilityPercentage:requiredViewVisibilityPercentage];
        }
        _impressionTimer.delegate = self;

        [properties removeObjectsForKeys:@[kImpressionTrackerURLsKey, kClickTrackerURLKey, kDefaultActionURLKey, kNativeAdConfigKey]];
        _properties = properties;

        if (!valid) {
            return nil;
        }

        // Add the DAA icon settings to our properties dictionary.
        // Path will not change, so load path and image statically.
        static NSString *daaIconImagePath = nil;
        static UIImage *daaIconImage = nil;
        if (!daaIconImagePath || !daaIconImage) {
            daaIconImagePath = MPResourcePathForResource(kDAAIconImageName);
            daaIconImage = daaIconImagePath ? [UIImage imageWithContentsOfFile:daaIconImagePath] : nil;
        }
        if (daaIconImagePath) {
            [properties setObject:daaIconImagePath forKey:kAdDAAIconImageKey];
        }
        if (daaIconImage) {
            [properties setObject:daaIconImage forKey:kAdDAAIconUIImageKey];
        }

        _destinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
    }

    return self;
}

- (void)dealloc
{
    [_destinationDisplayAgent cancel];
    [_destinationDisplayAgent setDelegate:nil];
}

#pragma mark - <MPNativeAdAdapter>

- (void)willAttachToView:(UIView *)view
{
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

#pragma mark - <MPAdImpressionTimerDelegate>

- (void)adViewWillLogImpression:(UIView *)adView
{
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

// - (MPAdConfiguration *)adConfiguration delegate method is automatically implemented via the adConfiguration property declaration.

@end
