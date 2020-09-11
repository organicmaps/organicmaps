//
//  MPNativeAd.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAd+Internal.h"
#import "MoPub+Utility.h"
#import "MPAdConfiguration.h"
#import "MPCoreInstanceProvider.h"
#import "MPNativeAdError.h"
#import "MPLogging.h"
#import "MPNativeCache.h"
#import "MPNativeAdRendering.h"
#import "MPImageDownloadQueue.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "MPNativeCustomEvent.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPTimer.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeView.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"
#import "MPImpressionTrackedNotification.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativeAd () <MPNativeAdAdapterDelegate, MPNativeViewDelegate>

@property (nonatomic, readwrite, strong) id<MPNativeAdRenderer> renderer;
@property (nonatomic, readwrite, strong) MPAdConfiguration *configuration;
@property (nonatomic, readwrite, strong) NSString *adUnitID;

@property (nonatomic, strong) NSDate *creationDate;

@property (nonatomic, strong) NSMutableSet *clickTrackerURLs;
@property (nonatomic, strong) NSMutableSet *impressionTrackerURLs;

@property (nonatomic, readonly, strong) id<MPNativeAdAdapter> adAdapter;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;

@property (nonatomic, copy) NSString *adIdentifier;
@property (nonatomic) MPNativeView *associatedView;

@property (nonatomic) BOOL hasAttachedToView;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPNativeAd

- (instancetype)initWithAdAdapter:(id<MPNativeAdAdapter>)adAdapter
{
    static int sequenceNumber = 0;

    self = [super init];
    if (self) {
        _adAdapter = adAdapter;
        if ([_adAdapter respondsToSelector:@selector(setDelegate:)]) {
            [_adAdapter setDelegate:self];
        }
        _adIdentifier = [[NSString stringWithFormat:@"%d", sequenceNumber++] copy];
        _impressionTrackerURLs = [[NSMutableSet alloc] init];
        _clickTrackerURLs = [[NSMutableSet alloc] init];
        _creationDate = [NSDate date];
        _associatedView = [[MPNativeView alloc] init];
        _associatedView.clipsToBounds = YES;
        _associatedView.delegate = self;

        // Add a tap recognizer on top of the view if the ad network isn't handling clicks on its own.
        if (!([_adAdapter respondsToSelector:@selector(enableThirdPartyClickTracking)] && [_adAdapter enableThirdPartyClickTracking])) {
            UITapGestureRecognizer *recognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(adViewTapped)];
            [_associatedView addGestureRecognizer:recognizer];
        }
    }
    return self;
}

#pragma mark - Public

- (UIView *)retrieveAdViewWithError:(NSError **)error
{
    // We always return the same MPNativeView (self.associatedView) so we need to remove its subviews
    // before attaching the new ad view to it. Also need to reset the `hasAttachedToView` state
    // variable back to `NO` since all of the subviews should be removed.
    for (UIView * view in self.associatedView.subviews) {
        [view removeFromSuperview];
    }

    if (self.associatedView.subviews.count == 0) {
        self.hasAttachedToView = NO;
    }

    UIView *adView = [self.renderer retrieveViewWithAdapter:self.adAdapter error:error];

    if (adView) {
        if (!self.hasAttachedToView) {
            [self willAttachToView:self.associatedView withAdContentViews:adView.subviews];
            self.hasAttachedToView = YES;
        }

        adView.frame = self.associatedView.bounds;
        [self.associatedView addSubview:adView];

        return self.associatedView;
    } else {
        return nil;
    }
}

- (NSDictionary *)properties
{
    return self.adAdapter.properties;
}

- (void)trackImpression
{
    if (self.hasTrackedImpression) {
        MPLogDebug(@"Impression already tracked.");
        return;
    }

    MPLogDebug(@"Tracking an impression for %@.", self.adIdentifier);
    self.hasTrackedImpression = YES;
    [self trackMetricsForURLs:self.impressionTrackerURLs];

    [MoPub sendImpressionDelegateAndNotificationFromAd:self
                                              adUnitID:self.adUnitID
                                        impressionData:self.configuration.impressionData];
}

- (void)trackClick
{
    if (self.hasTrackedClick) {
        MPLogDebug(@"Click already tracked.");
        return;
    }

    MPLogDebug(@"Tracking a click for %@.", self.adIdentifier);
    self.hasTrackedClick = YES;
    [self trackMetricsForURLs:self.clickTrackerURLs];

    if ([self.adAdapter respondsToSelector:@selector(trackClick)] && ![self isThirdPartyHandlingClicks]) {
        [self.adAdapter trackClick];
    }

}

- (void)trackMetricsForURLs:(NSSet *)URLs
{
    for (NSURL *URL in URLs) {
        [self trackMetricForURL:URL];
    }
}

- (void)trackMetricForURL:(NSURL *)URL
{
    MPURLRequest *request = [[MPURLRequest alloc] initWithURL:URL];
    [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:nil errorHandler:nil];
}

#pragma mark - Internal

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews {
    if ([self.adAdapter respondsToSelector:@selector(willAttachToView:withAdContentViews:)]) {
        [self.adAdapter willAttachToView:view withAdContentViews:adContentViews];
    } else if ([self.adAdapter respondsToSelector:@selector(willAttachToView:)]) {
        [self.adAdapter willAttachToView:view];
    }
}

- (BOOL)isThirdPartyHandlingClicks
{
    return [self.adAdapter respondsToSelector:@selector(enableThirdPartyClickTracking)] && [self.adAdapter enableThirdPartyClickTracking];
}

- (void)displayAdContent
{
    [self trackClick];

    if ([self.adAdapter respondsToSelector:@selector(displayContentForURL:rootViewController:)]) {
        [self.adAdapter displayContentForURL:self.adAdapter.defaultActionURL rootViewController:[self.delegate viewControllerForPresentingModalView]];
    } else {
        // If this method is called, that means that the backing adapter should implement -displayContentForURL:rootViewController:completion:.
        // If it doesn't, we'll log a warning.
        MPLogInfo(@"Cannot display native ad content. -displayContentForURL:rootViewController:completion: not implemented by native ad adapter: %@", [self.adAdapter class]);
    }
}

#pragma mark - UITapGestureRecognizer

- (void)adViewTapped
{
    [self displayAdContent];

    if ([self.renderer respondsToSelector:@selector(nativeAdTapped)]) {
        [self.renderer nativeAdTapped];
    }
}

#pragma mark - MPNativeViewDelegate

- (void)nativeViewWillMoveToSuperview:(UIView *)superview
{
    if ([self.renderer respondsToSelector:@selector(adViewWillMoveToSuperview:)])
    {
        [self.renderer adViewWillMoveToSuperview:superview];
    }
}

#pragma mark - MPNativeAdAdapterDelegate

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)nativeAdWillLogImpression:(id<MPNativeAdAdapter>)adAdapter
{
    [self trackImpression];
}

- (void)nativeAdDidClick:(id<MPNativeAdAdapter>)adAdapter
{
    MPLogAdEvent(MPLogEvent.adTapped, self.adIdentifier);
    [self trackClick];
}

- (void)nativeAdWillPresentModalForAdapter:(id<MPNativeAdAdapter>)adapter
{
    MPLogAdEvent(MPLogEvent.adWillPresentModal, self.adIdentifier);
    if ([self.delegate respondsToSelector:@selector(willPresentModalForNativeAd:)]) {
        [self.delegate willPresentModalForNativeAd:self];
    }
}

- (void)nativeAdDidDismissModalForAdapter:(id<MPNativeAdAdapter>)adapter
{
    MPLogAdEvent(MPLogEvent.adDidDismissModal, self.adIdentifier);
    if ([self.delegate respondsToSelector:@selector(didDismissModalForNativeAd:)]) {
        [self.delegate didDismissModalForNativeAd:self];
    }
}

- (void)nativeAdWillLeaveApplicationFromAdapter:(id<MPNativeAdAdapter>)adapter
{
    MPLogAdEvent(MPLogEvent.adWillLeaveApplication, self.adIdentifier);
    if ([self.delegate respondsToSelector:@selector(willLeaveApplicationFromNativeAd:)]) {
        [self.delegate willLeaveApplicationFromNativeAd:self];
    }
}

@end
