//
//  MPMoPubNativeCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubNativeCustomEvent.h"
#import "MPMoPubNativeAdAdapter.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdError.h"
#import "MPNativeAdConstants.h"
#import "MPLogging.h"
#import "MPNativeAdUtils.h"

@implementation MPMoPubNativeCustomEvent

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    NSString * adUnitId = info[kNativeAdUnitId];
    MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:info[kNativeAdDspCreativeId] dspName:info[kNativeAdDspName]], adUnitId);

    MPMoPubNativeAdAdapter *adAdapter = [[MPMoPubNativeAdAdapter alloc] initWithAdProperties:[info mutableCopy]];

    if (adAdapter.properties) {
        MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adAdapter];
        [interfaceAd.impressionTrackerURLs addObjectsFromArray:adAdapter.impressionTrackerURLs];
        [interfaceAd.clickTrackerURLs addObjectsFromArray:adAdapter.clickTrackerURLs];

        // Get the image urls so we can download them prior to returning the ad.
        NSMutableArray *imageURLs = [NSMutableArray array];
        for (NSString *key in [info allKeys]) {
            if ([[key lowercaseString] hasSuffix:@"image"] && [[info objectForKey:key] isKindOfClass:[NSString class]]) {
                NSString * urlString = [info objectForKey:key];
                // Empty URL string is acceptable. We only care about non-empty string that is not a valid URL.
                if (urlString.length != 0
                    && ![MPNativeAdUtils addURLString:urlString toURLArray:imageURLs]) {
                    NSError * error = MPNativeAdNSErrorForInvalidImageURL();
                    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], adUnitId);
                    [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:error];
                }
            }
        }

        [super precacheImagesWithURLs:imageURLs completionBlock:^(NSArray *errors) {
            if (errors) {
                NSError * error = MPNativeAdNSErrorForImageDownloadFailure();
                MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], adUnitId);
                [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:error];
            } else {
                MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], adUnitId);
                [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
            }
        }];
    } else {
        NSError * error = MPNativeAdNSErrorForInvalidAdServerResponse(nil);
        MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], adUnitId);
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:error];
    }

}

@end
