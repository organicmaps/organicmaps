//
//  FacebookBannerCustomEvent.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookBannerCustomEvent.h"

#import "MPInstanceProvider.h"
#import "MPLogging.h"

@interface MPInstanceProvider (FacebookBanners)

- (FBAdView *)buildFBAdViewWithPlacementID:(NSString *)placementID
                                      size:(FBAdSize)size
                        rootViewController:(UIViewController *)controller
                                  delegate:(id<FBAdViewDelegate>)delegate;
@end

@implementation MPInstanceProvider (FacebookBanners)

- (FBAdView *)buildFBAdViewWithPlacementID:(NSString *)placementID
                                      size:(FBAdSize)size
                        rootViewController:(UIViewController *)controller
                                  delegate:(id<FBAdViewDelegate>)delegate
{
    FBAdView *adView = [[FBAdView alloc] initWithPlacementID:placementID
                                                      adSize:size
                                          rootViewController:controller];
    adView.delegate = delegate;
    [adView disableAutoRefresh];
    return adView;
}

@end

@interface FacebookBannerCustomEvent () <FBAdViewDelegate>

@property (nonatomic, strong) FBAdView *fbAdView;

@end

@implementation FacebookBannerCustomEvent

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    return NO;
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info
{
    /**
     * Facebook Banner ads can accept arbitrary widths for given heights of 50 and 90. We convert these sizes
     * to Facebook's constants and set the fbAdView's size to the intended size ("size" passed to this method).
     */
    FBAdSize fbAdSize;
    if (CGSizeEqualToSize(size, kFBAdSize320x50.size)) {
        fbAdSize = kFBAdSize320x50;
    } else if (size.height == kFBAdSizeHeight250Rectangle.size.height) {
        fbAdSize = kFBAdSizeHeight250Rectangle;
    } else if (size.height == kFBAdSizeHeight90Banner.size.height) {
        fbAdSize = kFBAdSizeHeight90Banner;
    } else if (size.height == kFBAdSizeHeight50Banner.size.height) {
        fbAdSize = kFBAdSizeHeight50Banner;
    } else {
        MPLogError(@"Invalid size for Facebook banner ad");
        [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:nil];
        return;
    }

    if (![info objectForKey:@"placement_id"]) {
        MPLogError(@"Placement ID is required for Facebook banner ad");
        [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:nil];
        return;
    }

    MPLogInfo(@"Requesting Facebook banner ad");
    self.fbAdView =
        [[MPInstanceProvider sharedProvider] buildFBAdViewWithPlacementID:[info objectForKey:@"placement_id"]
                                                                     size:fbAdSize
                                                       rootViewController:[self.delegate viewControllerForPresentingModalView]
                                                                 delegate:self];

    if (!self.fbAdView) {
        [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:nil];
        return;
    }

    /*
     * Manually resize the frame of the FBAdView due to a bug in the Facebook SDK that sets the ad's width
     * to the width of the device instead of the width of the container it's placed in.
     * (Confirmed in email with a FB Solutions Engineer)
     */
    CGRect fbAdFrame = self.fbAdView.frame;
    fbAdFrame.size = size;
    self.fbAdView.frame = fbAdFrame;

    [self.fbAdView loadAd];
}

- (void)dealloc
{
    _fbAdView.delegate = nil;
}

#pragma mark FBAdViewDelegate methods

- (void)adView:(FBAdView *)adView didFailWithError:(NSError *)error;
{
    MPLogInfo(@"Facebook banner failed to load with error: %@", error.description);
    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];
}

- (void)adViewDidLoad:(FBAdView *)adView;
{
    MPLogInfo(@"Facebook banner ad did load");
    [self.delegate trackImpression];
    [self.delegate bannerCustomEvent:self didLoadAd:adView];
}

- (void)adViewDidClick:(FBAdView *)adView
{
    MPLogInfo(@"Facebook banner ad was clicked");
    [self.delegate trackClick];
    [self.delegate bannerCustomEventWillBeginAction:self];
}

- (void)adViewDidFinishHandlingClick:(FBAdView *)adView
{
    MPLogInfo(@"Facebook banner ad did finish handling click");
    [self.delegate bannerCustomEventDidFinishAction:self];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

@end
