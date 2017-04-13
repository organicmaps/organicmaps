//
//  MPNativeAdRequestTargeting.m
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAdRequestTargeting.h"
#import "MPNativeAdConstants.h"

#import <CoreLocation/CoreLocation.h>

@implementation MPNativeAdRequestTargeting

+ (MPNativeAdRequestTargeting *)targeting
{
    return [[MPNativeAdRequestTargeting alloc] init];
}

- (void)setDesiredAssets:(NSSet *)desiredAssets
{
    if (_desiredAssets != desiredAssets) {

        NSMutableSet *allowedAdAssets = [NSMutableSet setWithObjects:kAdTitleKey,
                                         kAdTextKey,
                                         kAdIconImageKey,
                                         kAdMainImageKey,
                                         kAdCTATextKey,
                                         kAdStarRatingKey,
                                         nil];
        [allowedAdAssets intersectSet:desiredAssets];
        _desiredAssets = allowedAdAssets;
    }
}


@end
