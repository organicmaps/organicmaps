//
//  MPMoPubNativeAdAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdDestinationDisplayAgent.h"
#import "MPAdImpressionTimer.h"
#import "MPCoreInstanceProvider.h"
#import "MPGlobal.h"
#import "MPMemoryCache.h"
#import "MPMoPubNativeAdAdapter.h"
#import "MPNativeAdConfigValues.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"

static const NSTimeInterval kMoPubRequiredSecondsForImpression = 1.0;
static const CGFloat kMoPubRequiredViewVisibilityPercentage = 0.5;

@interface MPMoPubNativeAdAdapter () <MPAdDestinationDisplayAgentDelegate, MPAdImpressionTimerDelegate>

@property (nonatomic, strong) MPAdImpressionTimer *impressionTimer;
@property (nonatomic, strong) id<MPAdDestinationDisplayAgent> destinationDisplayAgent;

@end

@implementation MPMoPubNativeAdAdapter

// synthesize for `MPNativeAdAdapter` protocol
@synthesize properties = _properties;
@synthesize defaultActionURL = _defaultActionURL;

- (instancetype)initWithAdProperties:(NSMutableDictionary *)properties
{
    if (self = [super init]) {

        // Let's make sure the data types of all the provided native ad properties are strings before creating the adapter

        NSArray *stringKeysToCheck = @[kAdIconImageKey, kAdMainImageKey, kAdTextKey, kAdSponsoredByCompanyKey, kAdTitleKey, kAdCTATextKey, kAdPrivacyIconImageUrlKey, kAdPrivacyIconClickUrlKey];

        for (NSString *key in stringKeysToCheck) {
            id value = properties[key];
            if (value != nil && ![value isKindOfClass:[NSString class]]) {
                return nil;
            }
        }

        // Validate that the views are actually views
        NSArray * viewKeysToCheck = @[kAdIconImageViewKey, kAdMainMediaViewKey];
        for (NSString * key in viewKeysToCheck) {
            id value = properties[key];
            if (value != nil && ![value isKindOfClass:[UIView class]]) {
                return nil;
            }
        }

        BOOL valid = YES;
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

        [properties removeObjectsForKeys:@[kClickTrackerURLKey, kDefaultActionURLKey, kNativeAdConfigKey]];
        _properties = properties;

        if (!valid) {
            return nil;
        }

        // The privacy icon has been overridden by the server. We will use its image instead if it is
        // already cached. Otherwise, we will defer loading the image until later.
        NSString * privacyIconUrl = properties[kAdPrivacyIconImageUrlKey];
        if (privacyIconUrl != nil) {
            UIImage * cachedIcon = [MPMemoryCache.sharedInstance imageForKey:privacyIconUrl];
            if (cachedIcon != nil) {
                [properties setObject:cachedIcon forKey:kAdPrivacyIconUIImageKey];
            }
        }
        // Use the default MoPub privacy icon bundled with the SDK.
        else {
            // Add the privacy icon settings to our properties dictionary.
            // Path will not change, so load path and image statically.
            static NSString *privacyIconImagePath = nil;
            static UIImage *privacyIconImage = nil;
            if (!privacyIconImagePath || !privacyIconImage) {
                privacyIconImagePath = MPResourcePathForResource(kPrivacyIconImageName);
                privacyIconImage = privacyIconImagePath ? [UIImage imageWithContentsOfFile:privacyIconImagePath] : nil;
            }
            if (privacyIconImagePath) {
                [properties setObject:privacyIconImagePath forKey:kAdPrivacyIconImageUrlKey];
            }
            if (privacyIconImage) {
                [properties setObject:privacyIconImage forKey:kAdPrivacyIconUIImageKey];
            }
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

#pragma mark - Privacy Icon

- (void)displayContentForDAAIconTap
{
    NSURL *defaultPrivacyClickUrl = [NSURL URLWithString:kPrivacyIconTapDestinationURL];
    NSURL *overridePrivacyClickUrl = ({
        NSString *url = self.properties[kAdPrivacyIconClickUrlKey];
        (url != nil ? [NSURL URLWithString:url] : nil);
    });

    [self.destinationDisplayAgent displayDestinationForURL:(overridePrivacyClickUrl != nil ? overridePrivacyClickUrl : defaultPrivacyClickUrl)];
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
