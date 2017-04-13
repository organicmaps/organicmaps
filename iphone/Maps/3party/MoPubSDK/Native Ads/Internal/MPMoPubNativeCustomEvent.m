//
//  MPMoPubNativeCustomEvent.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPMoPubNativeCustomEvent.h"
#import "MPMoPubNativeAdAdapter.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdError.h"
#import "MPLogging.h"
#import "MPNativeAdUtils.h"

@implementation MPMoPubNativeCustomEvent

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info
{
    MPMoPubNativeAdAdapter *adAdapter = [[MPMoPubNativeAdAdapter alloc] initWithAdProperties:[info mutableCopy]];

    if (adAdapter.properties) {
        MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adAdapter];
        [interfaceAd.impressionTrackerURLs addObjectsFromArray:adAdapter.impressionTrackerURLs];
        [interfaceAd.clickTrackerURLs addObjectsFromArray:adAdapter.clickTrackerURLs];

        // Get the image urls so we can download them prior to returning the ad.
        NSMutableArray *imageURLs = [NSMutableArray array];
        for (NSString *key in [info allKeys]) {
            if ([[key lowercaseString] hasSuffix:@"image"] && [[info objectForKey:key] isKindOfClass:[NSString class]]) {
                if (![MPNativeAdUtils addURLString:[info objectForKey:key] toURLArray:imageURLs]) {
                    [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidImageURL()];
                }
            }
        }

        [super precacheImagesWithURLs:imageURLs completionBlock:^(NSArray *errors) {
            if (errors) {
                MPLogDebug(@"%@", errors);
                [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForImageDownloadFailure()];
            } else {
                [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
            }
        }];
    } else {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(nil)];
    }

}

@end
