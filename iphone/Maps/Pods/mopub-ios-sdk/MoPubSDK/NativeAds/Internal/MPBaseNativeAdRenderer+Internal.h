//
//  MPBaseNativeAdRenderer+Internal.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseNativeAdRenderer.h"
#import "MPNativeAdRendering.h"

NS_ASSUME_NONNULL_BEGIN

@interface MPBaseNativeAdRenderer (Internal)

- (void)renderSponsoredByTextWithAdapter:(id<MPNativeAdAdapter>)adapter;

@property (nonatomic) UIView<MPNativeAdRendering> *adView;
@property (nonatomic) Class renderingViewClass;

@end

NS_ASSUME_NONNULL_END
