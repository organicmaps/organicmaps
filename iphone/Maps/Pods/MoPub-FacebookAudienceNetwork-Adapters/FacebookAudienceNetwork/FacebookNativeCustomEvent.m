//
//  FacebookNativeCustomEvent.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//
#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookNativeCustomEvent.h"
#import "FacebookNativeAdAdapter.h"
#import "FacebookAdapterConfiguration.h"
#if __has_include("MoPub.h")
    #import "MoPub.h"
    #import "MPNativeAd.h"
    #import "MPLogging.h"
    #import "MPNativeAdError.h"
#endif

static const NSInteger FacebookNoFillErrorCode = 1001;

@interface FacebookNativeCustomEvent () <FBNativeAdDelegate, FBNativeBannerAdDelegate>

@property (nonatomic, readwrite, strong) FBNativeAdBase *fbNativeAdBase;
@property (nonatomic, copy) NSString *fbPlacementId;
@property (nonatomic) Boolean isNativeBanner;

@end

@implementation FacebookNativeCustomEvent

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info
{
    [self requestAdWithCustomEventInfo:info adMarkup:nil];
}

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
     self.fbPlacementId = [info objectForKey:@"placement_id"];

    if (self.fbPlacementId) {
        if (self.localExtras != nil && [self.localExtras count] > 0) {
            self.isNativeBanner = [[self.localExtras objectForKey:@"native_banner"] boolValue];
        }
        
        self.isNativeBanner = self.isNativeBanner == nil ? FacebookAdapterConfiguration.isNativeBanner : self.isNativeBanner;
        
        if (self.isNativeBanner != nil) {
            if (self.isNativeBanner) {
                self.fbNativeAdBase = [[FBNativeBannerAd alloc] initWithPlacementID:self.fbPlacementId];
                    ((FBNativeBannerAd *) self.fbNativeAdBase).delegate = self;

                [self loadAdWithMarkup:adMarkup];

                return;
            }
        }
        
        self.fbNativeAdBase = [[FBNativeAd alloc] initWithPlacementID:self.fbPlacementId];
        ((FBNativeAd *) self.fbNativeAdBase).delegate = self;

        [self loadAdWithMarkup:adMarkup];
    } else {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid Facebook placement ID")];
        MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid Facebook placement ID")], self.fbPlacementId);
    }
}

- (void)loadAdWithMarkup:(NSString *)markup
{
    // Load the advanced bid payload.
    if (markup != nil) {
        MPLogInfo(@"Loading Facebook native ad markup for Advanced Bidding");
        [self.fbNativeAdBase loadAdWithBidPayload:markup];

        MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:nil dspName:nil], self.fbPlacementId);
    }
    else {
        MPLogInfo(@"Loading Facebook native ad");
        [self.fbNativeAdBase loadAd];

        MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:nil dspName:nil], self.fbPlacementId);
    }
    
    [FBAdSettings setMediationService:[FacebookAdapterConfiguration mediationString]];
}

#pragma mark - FBNativeAdDelegate

- (void)nativeAdDidLoad:(FBNativeAd *)nativeAd
{
    FacebookNativeAdAdapter *adAdapter = [[FacebookNativeAdAdapter alloc] initWithFBNativeAdBase:nativeAd adProperties:nil];
    MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adAdapter];

    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);
    [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
}

- (void)nativeAd:(FBNativeAd *)nativeAd didFailWithError:(NSError *)error
{
    if (error.code == FacebookNoFillErrorCode) {
        MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(self.class) error:MPNativeAdNSErrorForNoInventory()], self.fbPlacementId);
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForNoInventory()];
        
    } else {
        MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(self.class) error:MPNativeAdNSErrorForInvalidAdServerResponse(@"Facebook ad load error")], self.fbPlacementId);
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(@"Facebook ad load error")];
    }
}

#pragma mark - FBNativeBannerAdDelegate

- (void)nativeBannerAdDidLoad:(FBNativeBannerAd *)nativeBannerAd
{
    FacebookNativeAdAdapter *adAdapter = [[FacebookNativeAdAdapter alloc] initWithFBNativeAdBase:nativeBannerAd adProperties:nil];
    MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adAdapter];
    
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);
    [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
}

- (void)nativeBannerAd:(FBNativeBannerAd *)nativeBannerAd didFailWithError:(NSError *)error
{
    if (error.code == FacebookNoFillErrorCode) {
        MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(self.class) error:MPNativeAdNSErrorForNoInventory()], self.fbPlacementId);
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForNoInventory()];
        
    } else {
        MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(self.class) error:MPNativeAdNSErrorForInvalidAdServerResponse(@"Facebook ad load error")], self.fbPlacementId);
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(@"Facebook ad load error")];
    }
}

@end
