//
//  MPAdView.m
//  MoPub
//
//  Created by Nafis Jamal on 1/19/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPAdView.h"
#import "MPClosableView.h"
#import "MPBannerAdManager.h"
#import "MPInstanceProvider.h"
#import "MPBannerAdManagerDelegate.h"
#import "MPLogging.h"

@interface MPAdView () <MPBannerAdManagerDelegate>

@property (nonatomic, strong) MPBannerAdManager *adManager;
@property (nonatomic, weak) UIView *adContentView;
@property (nonatomic, assign) CGSize originalSize;
@property (nonatomic, assign) MPNativeAdOrientation allowedNativeAdOrientation;

@end

@implementation MPAdView
@synthesize location = _location;
@synthesize adManager = _adManager;
@synthesize adUnitId = _adUnitId;
@synthesize keywords = _keywords;
@synthesize delegate = _delegate;
@synthesize originalSize = _originalSize;
@synthesize testing = _testing;
@synthesize adContentView = _adContentView;
@synthesize allowedNativeAdOrientation = _allowedNativeAdOrientation;

#pragma mark -
#pragma mark Lifecycle

- (id)initWithAdUnitId:(NSString *)adUnitId size:(CGSize)size
{
    CGRect f = (CGRect){{0, 0}, size};
    if (self = [super initWithFrame:f])
    {
        self.backgroundColor = [UIColor clearColor];
        self.clipsToBounds = YES;
        self.originalSize = size;
        self.allowedNativeAdOrientation = MPNativeAdOrientationAny;
        self.adUnitId = (adUnitId) ? adUnitId : DEFAULT_PUB_ID;
        self.adManager = [[MPInstanceProvider sharedProvider] buildMPBannerAdManagerWithDelegate:self];
    }
    return self;
}

- (void)dealloc
{
    self.adManager.delegate = nil;
}

#pragma mark -

- (void)setAdContentView:(UIView *)view
{
    [self.adContentView removeFromSuperview];
    _adContentView = view;
    [self addSubview:view];
}

- (CGSize)adContentViewSize
{
    // MPClosableView represents an MRAID ad.
    if (!self.adContentView || [self.adContentView isKindOfClass:[MPClosableView class]]) {
        return self.originalSize;
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
    [self.adManager loadAd];
}

- (void)refreshAd
{
    [self loadAd];
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
    return self.originalSize;
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)invalidateContentView
{
    [self setAdContentView:nil];
}

- (void)managerDidFailToLoadAd
{
    if ([self.delegate respondsToSelector:@selector(adViewDidFailToLoadAd:)]) {
        // make sure we are not released synchronously as objects owned by us
        // may do additional work after this callback
        [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:self];

        [self.delegate adViewDidFailToLoadAd:self];
    }
}

- (void)managerDidLoadAd:(UIView *)ad
{
    [self setAdContentView:ad];
    if ([self.delegate respondsToSelector:@selector(adViewDidLoadAd:)]) {
        [self.delegate adViewDidLoadAd:self];
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

@end
