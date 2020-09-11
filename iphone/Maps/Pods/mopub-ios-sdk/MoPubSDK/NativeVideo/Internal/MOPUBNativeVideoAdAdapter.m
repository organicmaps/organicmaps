//
//  MOPUBNativeVideoAdAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBNativeVideoAdAdapter.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPAdImpressionTimer.h"
#import "MPCoreInstanceProvider.h"
#import "MPLogging.h"
#import "MPMemoryCache.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"

@interface MOPUBNativeVideoAdAdapter() <MPAdDestinationDisplayAgentDelegate, MPAdImpressionTimerDelegate>

@property (nonatomic) MPAdImpressionTimer *impressionTimer;
@property (nonatomic, strong) id<MPAdDestinationDisplayAgent> destinationDisplayAgent;

@end

@implementation MOPUBNativeVideoAdAdapter

// synthesize for `MPNativeAdAdapter` protocol
@synthesize properties = _properties;
@synthesize defaultActionURL = _defaultActionURL;

- (instancetype)initWithAdProperties:(NSMutableDictionary *)properties
{
    if (self = [super init]) {

        // Let's make sure the data types of all the provided native ad properties are strings before creating the adapter.

        NSArray *stringKeysToCheck = @[kAdIconImageKey, kAdMainImageKey, kAdTextKey, kAdSponsoredByCompanyKey, kAdTitleKey, kAdCTATextKey, kVASTVideoKey, kAdPrivacyIconImageUrlKey, kAdPrivacyIconClickUrlKey];

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

        [properties removeObjectsForKeys:[NSArray arrayWithObjects:kClickTrackerURLKey, kDefaultActionURLKey, nil]];
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
