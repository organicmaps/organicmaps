//
//  FacebookNativeCustomEvent.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookNativeCustomEvent.h"
#import "FacebookNativeAdAdapter.h"
#import "MPNativeAd.h"
#import "MPNativeAdError.h"
#import "MPLogging.h"
#import "MPNativeAdConstants.h"

static const NSInteger FacebookNoFillErrorCode = 1001;
static BOOL gVideoEnabled = NO;

@interface FacebookNativeCustomEvent () <FBNativeAdDelegate>

@property (nonatomic, readwrite, strong) FBNativeAd *fbNativeAd;
@property (nonatomic) BOOL videoEnabled;

@end

@implementation FacebookNativeCustomEvent

+ (void)setVideoEnabled:(BOOL)enabled
{
    gVideoEnabled = enabled;
}

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info
{
    NSString *placementID = [info objectForKey:@"placement_id"];

    if ([info objectForKey:kFBVideoAdsEnabledKey] == nil) {
        self.videoEnabled = gVideoEnabled;
    } else {
        self.videoEnabled = [[info objectForKey:kFBVideoAdsEnabledKey] boolValue];
    }

    if (placementID) {
        _fbNativeAd = [[FBNativeAd alloc] initWithPlacementID:placementID];
        self.fbNativeAd.delegate = self;
        [self.fbNativeAd loadAd];
    } else {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid Facebook placement ID")];
    }
}

#pragma mark - FBNativeAdDelegate

- (void)nativeAdDidLoad:(FBNativeAd *)nativeAd
{
    FacebookNativeAdAdapter *adAdapter = [[FacebookNativeAdAdapter alloc] initWithFBNativeAd:nativeAd adProperties:@{kFBVideoAdsEnabledKey:@(self.videoEnabled)}];
    MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adAdapter];

    NSMutableArray *imageURLs = [NSMutableArray array];

    if (nativeAd.icon.url) {
        [imageURLs addObject:nativeAd.icon.url];
    }

    // If video is enabled, no need to load coverImage.
    if (!self.videoEnabled && nativeAd.coverImage.url) {
        [imageURLs addObject:nativeAd.coverImage.url];
    }

    [super precacheImagesWithURLs:imageURLs completionBlock:^(NSArray *errors) {
        if (errors) {
            MPLogDebug(@"%@", errors);
            [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForImageDownloadFailure()];
        } else {
            [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
        }
    }];
}

- (void)nativeAd:(FBNativeAd *)nativeAd didFailWithError:(NSError *)error
{
    if (error.code == FacebookNoFillErrorCode) {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForNoInventory()];
    } else {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(@"Facebook ad load error")];
    }
}

@end
