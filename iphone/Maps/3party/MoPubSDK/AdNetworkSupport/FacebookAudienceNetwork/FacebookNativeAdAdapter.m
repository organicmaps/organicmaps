//
//  FacebookNativeAdAdapter.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
#import "MPLogging.h"

NSString *const kFBVideoAdsEnabledKey = @"video_enabled";

@interface FacebookNativeAdAdapter () <FBNativeAdDelegate>

@property (nonatomic, readonly) FBNativeAd *fbNativeAd;
@property (nonatomic, readonly) FBAdChoicesView *adChoicesView;
@property (nonatomic, readonly) FBMediaView *mediaView;

@end

@implementation FacebookNativeAdAdapter

@synthesize properties = _properties;

- (instancetype)initWithFBNativeAd:(FBNativeAd *)fbNativeAd adProperties:(NSDictionary *)adProps
{
    if (self = [super init]) {
        _fbNativeAd = fbNativeAd;
        _fbNativeAd.delegate = self;

        NSMutableDictionary *properties;
        if (adProps) {
            properties = [NSMutableDictionary dictionaryWithDictionary:adProps];
        } else {
            properties = [NSMutableDictionary dictionary];
        }

        if (fbNativeAd.title) {
            [properties setObject:fbNativeAd.title forKey:kAdTitleKey];
        }

        if (fbNativeAd.body) {
            [properties setObject:fbNativeAd.body forKey:kAdTextKey];
        }

        if (fbNativeAd.callToAction) {
            [properties setObject:fbNativeAd.callToAction forKey:kAdCTATextKey];
        }

        if (fbNativeAd.icon.url.absoluteString) {
            [properties setObject:fbNativeAd.icon.url.absoluteString forKey:kAdIconImageKey];
        }

        if (fbNativeAd.placementID) {
            [properties setObject:fbNativeAd.placementID forKey:@"placementID"];
        }

        if (fbNativeAd.socialContext) {
            [properties setObject:fbNativeAd.socialContext forKey:@"socialContext"];
        }

        _properties = properties;

        _adChoicesView = [[FBAdChoicesView alloc] initWithNativeAd:fbNativeAd];
        _adChoicesView.backgroundShown = NO;

        // If video ad is enabled, use mediaView, otherwise use coverImage.
        if ([[_properties objectForKey:kFBVideoAdsEnabledKey] boolValue]) {
            _mediaView = [[FBMediaView alloc] initWithNativeAd:fbNativeAd];
        } else {
            if (fbNativeAd.coverImage.url.absoluteString) {
                [properties setObject:fbNativeAd.coverImage.url.absoluteString forKey:kAdMainImageKey];
            }
        }
    }

    return self;
}


#pragma mark - MPNativeAdAdapter

- (NSURL *)defaultActionURL
{
    return nil;
}

- (BOOL)enableThirdPartyClickTracking
{
    return YES;
}

- (void)willAttachToView:(UIView *)view
{
    [self.fbNativeAd registerViewForInteraction:view withViewController:[self.delegate viewControllerForPresentingModalView]];
}

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews
{
    if ( adContentViews.count > 0 ) {
        [self.fbNativeAd registerViewForInteraction:view withViewController:[self.delegate viewControllerForPresentingModalView] withClickableViews:adContentViews];
    } else {
        [self willAttachToView:view];
    }
}

- (UIView *)privacyInformationIconView
{
    return self.adChoicesView;
}

- (UIView *)mainMediaView
{
    return self.mediaView;
}

#pragma mark - FBNativeAdDelegate

- (void)nativeAdWillLogImpression:(FBNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLogImpression:)]) {
        [self.delegate nativeAdWillLogImpression:self];
    } else {
        MPLogWarn(@"Delegate does not implement impression tracking callback. Impressions likely not being tracked.");
    }
}

- (void)nativeAdDidClick:(FBNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidClick:)]) {
        [self.delegate nativeAdDidClick:self];
    } else {
        MPLogWarn(@"Delegate does not implement click tracking callback. Clicks likely not being tracked.");
    }

    [self.delegate nativeAdWillPresentModalForAdapter:self];
}

- (void)nativeAdDidFinishHandlingClick:(FBNativeAd *)nativeAd
{
    [self.delegate nativeAdDidDismissModalForAdapter:self];
}

@end
