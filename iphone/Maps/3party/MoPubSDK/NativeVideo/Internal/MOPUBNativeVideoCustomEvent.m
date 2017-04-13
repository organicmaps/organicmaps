//
//  MOPUBNativeVideoCustomEvent.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPNativeAd.h"
#import "MPNativeAdError.h"
#import "MPNativeAdUtils.h"
#import "MPNativeAdConstants.h"
#import "MOPUBNativeVideoAdAdapter.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MOPUBNativeVideoCustomEvent.h"
#import "MPLogging.h"
#import "MPVideoConfig.h"
#import "MPVASTManager.h"
#import "MPNativeAd+Internal.h"

@implementation MOPUBNativeVideoCustomEvent

- (void)handleSuccessfulVastParsing:(MPVASTResponse *)mpVastResponse info:(NSDictionary *)info
{
    NSMutableDictionary *infoMutableCopy = [info mutableCopy];
    [infoMutableCopy setObject:[[MPVideoConfig alloc] initWithVASTResponse:mpVastResponse additionalTrackers:((MOPUBNativeVideoAdConfigValues *)info[kNativeVideoAdConfigKey]).trackers] forKey:kVideoConfigKey];
    MOPUBNativeVideoAdAdapter *adAdapter = [[MOPUBNativeVideoAdAdapter alloc] initWithAdProperties:infoMutableCopy];
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

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info
{
    MOPUBNativeVideoAdConfigValues *nativeVideoAdConfigValues = [info objectForKey:kNativeVideoAdConfigKey];
    if (nativeVideoAdConfigValues && [nativeVideoAdConfigValues isValid]) {
        NSString *vastString = [info objectForKey:kVASTVideoKey];
        if (vastString) {
            [MPVASTManager fetchVASTWithData:[vastString dataUsingEncoding:NSUTF8StringEncoding]
                                  completion: ^(MPVASTResponse *mpVastResponse, NSError *error) {
                                      if (error) {
                                          [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForVASTParsingFailure()];
                                      } else {
                                          [self handleSuccessfulVastParsing:mpVastResponse info:info];
                                      }
                                  }];
        } else {
            [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForVASTParsingFailure()];
        }
    } else {
        [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:MPNativeAdNSErrorForVideoConfigInvalid()];
    }
}

@end
