//
//  MTRGNativeViewsFactory.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>


#import <MyTargetSDKCorp/MTRGNativeImageBanner.h>
#import <MyTargetSDKCorp/MTRGNativeTeaserBanner.h>
#import <MyTargetSDKCorp/MTRGNativePromoBanner.h>
#import <MyTargetSDKCorp/MTRGNativeAppwallBanner.h>

#import <MyTargetSDKCorp/MTRGNewsFeedAdView.h>
#import <MyTargetSDKCorp/MTRGChatListAdView.h>
#import <MyTargetSDKCorp/MTRGContentStreamAdView.h>
#import <MyTargetSDKCorp/MTRGContentWallAdView.h>

#import <MyTargetSDKCorp/MTRGAppwallBannerAdView.h>
#import <MyTargetSDKCorp/MTRGAppwallAdView.h>

@interface MTRGNativeViewsFactory : NSObject

//Тизер с кнопкой
+(MTRGNewsFeedAdView *) createNewsFeedViewWithBanner:(MTRGNativeTeaserBanner *)teaserBanner;
//Тизер
+(MTRGChatListAdView *) createChatListViewWithBanner:(MTRGNativeTeaserBanner *)teaserBanner;
//Промо
+(MTRGContentStreamAdView *) createContentStreamViewWithBanner:(MTRGNativePromoBanner *)promoBanner;
//Картинка
+(MTRGContentWallAdView *) createContentWallViewWithBanner:(MTRGNativeImageBanner *)imageBanner;

//App-wall-баннер
+(MTRGAppwallBannerAdView *) createAppWallBannerViewWithBanner:(MTRGNativeAppwallBanner *) appWallBanner;
//App-wall-таблица
+(MTRGAppwallAdView *) createAppWallAdViewWithBanners:(NSArray*)banners;

@end
