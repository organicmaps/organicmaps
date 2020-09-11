//
//  MPNativeAd+Internal.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPNativeAd+Internal.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeView.h"

@implementation MPNativeAd (Internal)

@dynamic impressionTrackerURLs;
@dynamic clickTrackerURLs;
@dynamic creationDate;
@dynamic adUnitID;
@dynamic renderer;
@dynamic configuration;
@dynamic associatedView;
@dynamic adAdapter;

- (void)updateAdViewSize:(CGSize)size
{
    self.associatedView.frame = CGRectMake(0, 0, size.width, size.height);
}

- (UIView *)retrieveAdViewForSizeCalculationWithError:(NSError **)error
{
    // retrieve the ad and apply the frame of the associatedView (superview of the adView) so the
    // adView can calculate its own size. It's important that we don't add adView to the associatedView
    // because this can mess up expectations in `retrieveAdViewWithError:` especially around hydrating
    // image views asynchronously
    UIView *adView = [self.renderer retrieveViewWithAdapter:self.adAdapter error:error];
    adView.frame = self.associatedView.bounds;
    return adView;
}

@end
