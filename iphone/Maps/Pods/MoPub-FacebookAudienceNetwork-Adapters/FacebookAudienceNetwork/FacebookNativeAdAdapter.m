//
//  FacebookNativeAdAdapter.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookNativeAdAdapter.h"
#if __has_include("MoPub.h")
    #import "MPNativeAdConstants.h"
    #import "MPNativeAdError.h"
    #import "MPLogging.h"
#endif

@interface FacebookNativeAdAdapter () <FBNativeAdDelegate, FBNativeBannerAdDelegate>

@property (nonatomic, readonly) FBAdOptionsView *adOptionsView;
@property (nonatomic, readonly) FBMediaView *mediaView;
@property (nonatomic, readonly) FBAdIconView *iconView;

@end

@implementation FacebookNativeAdAdapter

@synthesize properties = _properties;

- (instancetype)initWithFBNativeAdBase:(FBNativeAdBase *)fbNativeAdBase adProperties:(NSDictionary *)adProps
{
    if (self = [super init]) {
        _fbNativeAdBase = fbNativeAdBase;

        if ([_fbNativeAdBase class] == [FBNativeBannerAd class]){
            ((FBNativeBannerAd *) _fbNativeAdBase).delegate = self;
        } else if ([_fbNativeAdBase class] == [FBNativeAd class]){
            ((FBNativeAd *) _fbNativeAdBase).delegate = self;
        }
        _mediaView = [[FBMediaView alloc] init];
        _iconView = [[FBAdIconView alloc] init];

        NSMutableDictionary *properties;
        if (adProps) {
            properties = [NSMutableDictionary dictionaryWithDictionary:adProps];
        } else {
            properties = [NSMutableDictionary dictionary];
        }

        if (_fbNativeAdBase.headline) {
            [properties setObject:_fbNativeAdBase.headline forKey:kAdTitleKey];
        }

        if (_fbNativeAdBase.bodyText) {
            [properties setObject:_fbNativeAdBase.bodyText forKey:kAdTextKey];
        }

        if (_fbNativeAdBase.callToAction) {
            [properties setObject:_fbNativeAdBase.callToAction forKey:kAdCTATextKey];
        }
        
        /* Per Facebook's requirements, either the ad title or the advertiser name
        will be displayed, depending on the FB SDK version. Therefore, mapping both
        to the MoPub's ad title asset */
        if (_fbNativeAdBase.advertiserName) {
            [properties setObject:_fbNativeAdBase.advertiserName forKey:kAdTitleKey];
        }
        
        [properties setObject:_iconView forKey:kAdIconImageViewKey];
        [properties setObject:_mediaView forKey:kAdMainMediaViewKey];

        if (_fbNativeAdBase.placementID) {
            [properties setObject:_fbNativeAdBase.placementID forKey:@"placementID"];
        }

        if (_fbNativeAdBase.socialContext) {
            [properties setObject:_fbNativeAdBase.socialContext forKey:@"socialContext"];
        }
        
        if (_fbNativeAdBase.sponsoredTranslation) {
            [properties setObject:_fbNativeAdBase.sponsoredTranslation forKey:@"sponsoredTranslation"];
        }

        _properties = properties;

        _adOptionsView = [[FBAdOptionsView alloc] init];
        _adOptionsView.nativeAd = _fbNativeAdBase;
        _adOptionsView.backgroundColor = [UIColor clearColor];
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
    if ([_fbNativeAdBase class] == [FBNativeBannerAd class]) {
        [((FBNativeBannerAd *) self.fbNativeAdBase) registerViewForInteraction:view iconView:self.iconView viewController:[self.delegate viewControllerForPresentingModalView]];
    } else if ([_fbNativeAdBase class] == [FBNativeAd class]) {
        [((FBNativeAd *) self.fbNativeAdBase) registerViewForInteraction:view mediaView:self.mediaView iconView:self.iconView viewController:[self.delegate viewControllerForPresentingModalView]];
    }
}

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews
{
    if ( adContentViews.count > 0 ) {
        if ([_fbNativeAdBase class] == [FBNativeBannerAd class]){
             [((FBNativeBannerAd *) self.fbNativeAdBase) registerViewForInteraction:view iconView:self.iconView viewController:[self.delegate viewControllerForPresentingModalView] clickableViews:adContentViews];
         } else if ([_fbNativeAdBase class] == [FBNativeAd class]){
             [((FBNativeAd *) self.fbNativeAdBase) registerViewForInteraction:view mediaView:self.mediaView iconView:self.iconView viewController:[self.delegate viewControllerForPresentingModalView] clickableViews:adContentViews];
         }
    } else {
        [self willAttachToView:view];
    }
}

- (UIView *)privacyInformationIconView
{
    return self.adOptionsView;
}

- (UIView *)mainMediaView
{
    return self.mediaView;
}

- (UIView *)iconMediaView
{
    return self.iconView;
}

#pragma mark - FBNativeAdDelegate

- (void)nativeAdWillLogImpression:(FBNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLogImpression:)]) {
        MPLogAdEvent([MPLogEvent adShowSuccessForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);
        MPLogAdEvent([MPLogEvent adWillAppearForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);

        [self.delegate nativeAdWillLogImpression:self];
    } else {
        MPLogInfo(@"Delegate does not implement impression tracking callback. Impressions likely not being tracked.");
    }
}

- (void)nativeAdDidClick:(FBNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidClick:)]) {
        MPLogAdEvent([MPLogEvent adTappedForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);
        [self.delegate nativeAdDidClick:self];
    } else {
        MPLogInfo(@"Delegate does not implement click tracking callback. Clicks likely not being tracked.");
    }

    MPLogAdEvent([MPLogEvent adWillPresentModalForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);

    [self.delegate nativeAdWillPresentModalForAdapter:self];
}

- (void)nativeAdDidFinishHandlingClick:(FBNativeAd *)nativeAd
{
    MPLogAdEvent([MPLogEvent adDidDismissModalForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);

    [self.delegate nativeAdDidDismissModalForAdapter:self];
}

#pragma mark - FBNativeBannerAdDelegate

- (void)nativeBannerAdWillLogImpression:(FBNativeBannerAd *)nativeBannerAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLogImpression:)]) {
        MPLogAdEvent([MPLogEvent adShowSuccessForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);
        MPLogAdEvent([MPLogEvent adWillAppearForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);

        [self.delegate nativeAdWillLogImpression:self];
    } else {
        MPLogInfo(@"Delegate does not implement impression tracking callback. Impressions likely not being tracked.");
    }
}

- (void)nativeBannerAdDidClick:(FBNativeBannerAd *)nativeBannerAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidClick:)]) {
        MPLogAdEvent([MPLogEvent adTappedForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);

        [self.delegate nativeAdDidClick:self];
    } else {
        MPLogInfo(@"Delegate does not implement click tracking callback. Clicks likely not being tracked.");
    }
    
    MPLogAdEvent([MPLogEvent adWillPresentModalForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);
    
    [self.delegate nativeAdWillPresentModalForAdapter:self];
}

- (void)nativeBannerAdDidFinishHandlingClick:(FBNativeBannerAd *)nativeBannerAd
{
    MPLogAdEvent([MPLogEvent adDidDismissModalForAdapter:NSStringFromClass(self.class)], self.fbNativeAdBase.placementID);
    
    [self.delegate nativeAdDidDismissModalForAdapter:self];
}

@end
