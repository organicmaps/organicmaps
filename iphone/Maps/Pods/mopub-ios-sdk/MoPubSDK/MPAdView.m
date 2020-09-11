//
//  MPAdView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdView.h"
#import "MoPub+Utility.h"
#import "MPAdTargeting.h"
#import "MPBannerAdManager.h"
#import "MPBannerAdManagerDelegate.h"
#import "MPClosableView.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPGlobal.h"
#import "MPImpressionTrackedNotification.h"
#import "MPLogging.h"

@interface MPAdView () <MPBannerAdManagerDelegate>

@property (nonatomic, strong) MPBannerAdManager *adManager;
@property (nonatomic, weak) UIView *adContentView;
@property (nonatomic, assign) MPNativeAdOrientation allowedNativeAdOrientation;

@end

@implementation MPAdView

#pragma mark -
#pragma mark Lifecycle

- (id)initWithAdUnitId:(NSString *)adUnitId
{
    if (self = [super initWithFrame:CGRectZero])
    {
        self.backgroundColor = [UIColor clearColor];
        self.clipsToBounds = YES;
        self.maxAdSize = kMPPresetMaxAdSizeMatchFrame;
        self.allowedNativeAdOrientation = MPNativeAdOrientationAny;
        self.adUnitId = (adUnitId) ? adUnitId : DEFAULT_PUB_ID;
        self.adManager = [[MPBannerAdManager alloc] initWithDelegate:self];
        self.userInteractionEnabled = NO;
    }
    return self;
}

- (id)initWithAdUnitId:(NSString *)adUnitId size:(CGSize)size
{
    MPAdView * adView = [self initWithAdUnitId:adUnitId];
    adView.frame = ({
        CGRect frame = adView.frame;
        frame.size = [MPAdView sizeForContainer:adView adSize:size adUnitId:adUnitId];
        frame;
    });
    adView.maxAdSize = size;
    return adView;
}

- (void)dealloc
{
    self.adManager.delegate = nil;
}

- (void)layoutSubviews
{
    [super layoutSubviews];

    // Re-center the creative within this container only if the
    // creative isn't MRAID.
    if (!self.adManager.isMraidAd) {
        // Calculate the center using the bounds instead of the center property since bases its
        // center relative its superview which may not be correct.
        CGPoint center = CGPointMake(floorf(self.bounds.size.width / 2.0), floorf(self.bounds.size.height / 2.0));
        self.adContentView.center = center;
    }
}

#pragma mark -

- (void)setAdContentView:(UIView *)view
{
    [self.adContentView removeFromSuperview];
    _adContentView = view;

    if (view != nil) {
        [self addSubview:view];
        [self setNeedsLayout];

        self.userInteractionEnabled = YES;
    }
    else {
        self.userInteractionEnabled = NO;
    }
}

- (CGSize)adContentViewSize
{
    // MPClosableView represents an MRAID ad.
    if (!self.adContentView || [self.adContentView isKindOfClass:[MPClosableView class]]) {
        return [MPAdView sizeForContainer:self adSize:self.maxAdSize adUnitId:self.adUnitId];
    } else {
        return self.adContentView.bounds.size;
    }
}

- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation
{
    [self.adManager rotateToOrientation:newOrientation];
}

- (void)loadAd
{
    [self.adManager loadAdWithTargeting: self.adTargeting];
}

- (void)loadAdWithMaxAdSize:(CGSize)size
{
    // Update the maximum desired ad size
    self.maxAdSize = size;

    // Attempt to load an ad.
    [self loadAd];
}

- (void)refreshAd
{
    [self loadAdWithMaxAdSize:self.maxAdSize];
}

- (void)forceRefreshAd
{
    [self.adManager forceRefreshAd];
}

- (void)stopAutomaticallyRefreshingContents
{
    [self.adManager stopAutomaticallyRefreshingContents];
}

- (void)startAutomaticallyRefreshingContents
{
    [self.adManager startAutomaticallyRefreshingContents];
}

- (void)lockNativeAdsToOrientation:(MPNativeAdOrientation)orientation
{
    self.allowedNativeAdOrientation = orientation;
}

- (void)unlockNativeAdsOrientation
{
    self.allowedNativeAdOrientation = MPNativeAdOrientationAny;
}

- (MPNativeAdOrientation)allowedNativeAdsOrientation
{
    return self.allowedNativeAdOrientation;
}

#pragma mark - Sizing

/**
 Hydrates an ad size to an explicit ad size in points for a given ad container.
 If the size is already explicit, nothing will happen.
 @param container Container view for the ad
 @param adSize Ad size to rehydrate
 @param adUnitId Ad unit ID used for logging purposes
 @return Rehydrated ad size
 */
+ (CGSize)sizeForContainer:(UIView * _Nullable)container adSize:(CGSize)adSize adUnitId:(NSString * _Nullable)adUnitId
{
    // Hydrating an ad size means resolving the `kMPFlexibleAdSize` value
    // into it's final size value based upon the container bounds.
    CGSize hydratedAdSize = adSize;

    // Hydrate the width.
    if (adSize.width == kMPFlexibleAdSize) {
        // Frame hasn't been set, issue a warning.
        if (container.bounds.size.width == 0) {
            MPLogEvent * event = [MPLogEvent error:[NSError frameWidthNotSetForFlexibleSize] message:nil];
            [MPLogging logEvent:event source:adUnitId fromClass:self.class];
        }

        hydratedAdSize.width = container.bounds.size.width;
    }

    if (adSize.height == kMPFlexibleAdSize) {
        // Frame hasn't been set, issue a warning.
        if (container.bounds.size.height == 0) {
            MPLogEvent * event = [MPLogEvent error:[NSError frameHeightNotSetForFlexibleSize] message:nil];
            [MPLogging logEvent:event source:adUnitId fromClass:self.class];
        }

        hydratedAdSize.height = container.bounds.size.height;
    }

    return hydratedAdSize;
}

#pragma mark - <MPBannerAdManagerDelegate>

- (MPAdView *)banner
{
    return self;
}

- (id<MPAdViewDelegate>)bannerDelegate
{
    return self.delegate;
}

- (CGSize)containerSize
{
    return [MPAdView sizeForContainer:self adSize:self.maxAdSize adUnitId:self.adUnitId];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (MPAdTargeting *)adTargeting {
    // Generate the explicit creative safe area size.
    CGSize realSize = [MPAdView sizeForContainer:self adSize:self.maxAdSize adUnitId:self.adUnitId];

    // Build the targeting information
    MPAdTargeting * targeting = [MPAdTargeting targetingWithCreativeSafeSize:realSize];
    targeting.keywords = self.keywords;
    targeting.localExtras = self.localExtras;
    targeting.userDataKeywords = self.userDataKeywords;

    return targeting;
}

- (void)invalidateContentView
{
    [self setAdContentView:nil];
}

- (void)managerDidFailToLoadAdWithError:(NSError *)error
{
    if ([self.delegate respondsToSelector:@selector(adViewDidFailToLoadAd:)]) {
        // make sure we are not released synchronously as objects owned by us
        // may do additional work after this callback
        [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:self];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        [self.delegate adViewDidFailToLoadAd:self];
#pragma GCC diagnostic pop
    }
    if ([self.delegate respondsToSelector:@selector(adView:didFailToLoadAdWithError:)]) {
        // make sure we are not released synchronously as objects owned by us
        // may do additional work after this callback
        [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:self];
        [self.delegate adView:self didFailToLoadAdWithError:error];
    }
}

- (void)managerDidLoadAd:(UIView *)ad
{
    [self setAdContentView:ad];
    if ([self.delegate respondsToSelector:@selector(adViewDidLoadAd:)]) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        [self.delegate adViewDidLoadAd:self];
#pragma GCC diagnostic pop
    }

    if ([self.delegate respondsToSelector:@selector(adViewDidLoadAd:adSize:)]) {
        [self.delegate adViewDidLoadAd:self adSize:ad.bounds.size];
    }
}

- (void)userActionWillBegin
{
    if ([self.delegate respondsToSelector:@selector(willPresentModalViewForAd:)]) {
        [self.delegate willPresentModalViewForAd:self];
    }
}

- (void)userActionDidFinish
{
    if ([self.delegate respondsToSelector:@selector(didDismissModalViewForAd:)]) {
        [self.delegate didDismissModalViewForAd:self];
    }
}

- (void)userWillLeaveApplication
{
    if ([self.delegate respondsToSelector:@selector(willLeaveApplicationFromAd:)]) {
        [self.delegate willLeaveApplicationFromAd:self];
    }
}

- (void)impressionDidFireWithImpressionData:(MPImpressionData *)impressionData {
    [MoPub sendImpressionDelegateAndNotificationFromAd:self
                                              adUnitID:self.adUnitId
                                        impressionData:impressionData];
}

@end
