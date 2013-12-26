//
//  MPAdView.m
//  MoPub
//
//  Created by Nafis Jamal on 1/19/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPAdView.h"
#import "MRAdView.h"
#import "MPBannerAdManager.h"
#import "MPInstanceProvider.h"
#import "MPBannerAdManagerDelegate.h"
#import "MPLogging.h"

@interface MPAdView () <MPBannerAdManagerDelegate>

@property (nonatomic, retain) MPBannerAdManager *adManager;
@property (nonatomic, assign) UIView *adContentView;
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
@synthesize ignoresAutorefresh = _ignoresAutorefresh;
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
    self.delegate = nil;
    self.adUnitId = nil;
    self.location = nil;
    self.keywords = nil;
    self.adManager.delegate = nil;
    self.adManager = nil;
    [super dealloc];
}

#pragma mark -

- (void)setAdContentView:(UIView *)view
{
    [view retain];
    [self.adContentView removeFromSuperview];
    _adContentView = view;
    [self addSubview:view];
    [view release];
}

- (BOOL)ignoresAutorefresh
{
    return _ignoresAutorefresh;
}

- (void)setIgnoresAutorefresh:(BOOL)ignoresAutorefresh
{
    if (_ignoresAutorefresh != ignoresAutorefresh) {
        _ignoresAutorefresh = ignoresAutorefresh;
    }

    if (_ignoresAutorefresh) {
        [self.adManager stopAutomaticallyRefreshingContents];
    } else {
        [self.adManager startAutomaticallyRefreshingContents];
    }
}

- (CGSize)adContentViewSize
{
    if (!self.adContentView || [self.adContentView isKindOfClass:[MRAdView class]]) {
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
        // Make sure we're not deallocated immediately as a result of a delegate
        // action in reponse to this callback.
        [[self retain] autorelease];

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

# pragma mark - Deprecated Custom Events Mechanism

- (void)customEventDidLoadAd
{
    [self.adManager customEventDidLoadAd];
}

- (void)customEventDidFailToLoadAd
{
    [self.adManager customEventDidFailToLoadAd];
}

- (void)customEventActionWillBegin
{
    [self.adManager customEventActionWillBegin];
}

- (void)customEventActionDidEnd
{
    [self.adManager customEventActionDidEnd];
}

@end
