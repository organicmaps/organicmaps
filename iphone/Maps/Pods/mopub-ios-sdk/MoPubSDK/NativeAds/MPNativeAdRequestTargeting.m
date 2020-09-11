//
//  MPNativeAdRequestTargeting.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdRequestTargeting.h"
#import "MPNativeAdConstants.h"

#import <CoreLocation/CoreLocation.h>

@implementation MPNativeAdRequestTargeting

+ (MPNativeAdRequestTargeting *)targeting
{
    return [[MPNativeAdRequestTargeting alloc] initWithCreativeSafeSize:CGSizeZero];
}

- (void)setDesiredAssets:(NSSet *)desiredAssets
{
    if (_desiredAssets != desiredAssets) {

        NSMutableSet *allowedAdAssets = [NSMutableSet setWithObjects:kAdTitleKey,
                                         kAdTextKey,
                                         kAdSponsoredByCompanyKey,
                                         kAdIconImageKey,
                                         kAdMainImageKey,
                                         kAdCTATextKey,
                                         kAdStarRatingKey,
                                         kAdIconImageViewKey,
                                         kAdMainMediaViewKey,
                                         nil];
        [allowedAdAssets intersectSet:desiredAssets];
        _desiredAssets = allowedAdAssets;
    }
}


@end
