//
//  MTRGNativeViewsFactory.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>


#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGNativeAppwallBanner.h>
#import <MyTargetSDK/MTRGNewsFeedAdView.h>
#import <MyTargetSDK/MTRGChatListAdView.h>
#import <MyTargetSDK/MTRGContentStreamAdView.h>
#import <MyTargetSDK/MTRGContentWallAdView.h>

#import <MyTargetSDK/MTRGAppwallBannerAdView.h>
#import <MyTargetSDK/MTRGAppwallAdView.h>

#import <MyTargetSDK/MTRGMediaAdView.h>
#import <MyTargetSDK/MTRGContentStreamCardAdView.h>
#import <MyTargetSDK/MTRGPromoCardCollectionView.h>

@interface MTRGNativeViewsFactory : NSObject

+ (MTRGNewsFeedAdView *)createNewsFeedViewWithBanner:(MTRGNativePromoBanner *)teaserBanner;

+ (MTRGChatListAdView *)createChatListViewWithBanner:(MTRGNativePromoBanner *)teaserBanner;

+ (MTRGContentStreamAdView *)createContentStreamViewWithBanner:(MTRGNativePromoBanner *)promoBanner;

+ (MTRGContentWallAdView *)createContentWallViewWithBanner:(MTRGNativePromoBanner *)imageBanner;

+ (MTRGAppwallBannerAdView *)createAppWallBannerViewWithBanner:(MTRGNativeAppwallBanner *)appWallBanner
                                                      delegate:(id <MTRGAppwallBannerAdViewDelegate>)delegate;

+ (MTRGAppwallBannerAdView *)createAppWallBannerViewWithDelegate:(id <MTRGAppwallBannerAdViewDelegate>)delegate;

+ (MTRGAppwallAdView *)createAppWallAdViewWithBanners:(NSArray *)banners;

+ (MTRGMediaAdView *)createMediaAdView;

+ (MTRGContentStreamCardAdView *)createContentStreamCardAdView;

+ (MTRGPromoCardCollectionView *)createPromoCardCollectionView;

@end
