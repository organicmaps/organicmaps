//
//  MTRGAppwallBannerAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 15.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativeAppwallBanner.h>

@class MTRGAppwallBannerAdView;

@protocol MTRGAppwallBannerAdViewDelegate <NSObject>

- (void)appWallBannerAdViewOnClickWithView:(MTRGAppwallBannerAdView *)bannerAdView;

- (void)appWallBannerAdViewOnCancelSelectWithView:(MTRGAppwallBannerAdView *)bannerAdView;

@optional

- (BOOL)appWallBannerAdViewAllowStartSelect:(MTRGAppwallBannerAdView *)bannerAdView;

@end


@interface MTRGAppwallBannerAdView : UIView

@property(nonatomic) MTRGNativeAppwallBanner *appWallBanner;

- (instancetype)initWithDelegate:(id <MTRGAppwallBannerAdViewDelegate>)delegate;
- (void)setAppWallBanner:(MTRGNativeAppwallBanner *)appWallBanner;

- (void)setFixedWidth:(CGFloat)width;

@property(nonatomic) UIEdgeInsets titleMargins;
@property(nonatomic) UIEdgeInsets descriptionMargins;
@property(nonatomic) UIEdgeInsets iconMargins;
@property(nonatomic) UIEdgeInsets ratingStarsMargins;
@property(nonatomic) UIEdgeInsets votesMargins;
@property(nonatomic) UIEdgeInsets gotoAppMargins;
@property(nonatomic) UIEdgeInsets crossNotifMargins;
@property(nonatomic) UIEdgeInsets coinsViewMargins;
@property(nonatomic) UIEdgeInsets coinsTextMargins;

@property(nonatomic) UIEdgeInsets paddings;

@property(nonatomic) UIColor * touchColor;
@property(nonatomic) UIColor * normalColor;

@property(nonatomic) CGSize iconSize;
@property(nonatomic) CGSize ratingSize;
@property(nonatomic) CGSize coinViewSize;
@property(nonatomic) CGSize coinSize;
@property(nonatomic) CGSize gotoAppSize;
@property(nonatomic) CGSize statusImageSize;
@property(nonatomic) CGSize bubbleSize;
@property(nonatomic) CGSize crossNotifIconSize;

@property(nonatomic) CGPoint bubblePosition;

@property(nonatomic) UIFont * titleFont;
@property(nonatomic) UIFont * descriptionFont;
@property(nonatomic) UIFont * votesFont;
@property(nonatomic) UIFont * coinFont;

@property(nonatomic) NSInteger descriptionNumberOfLines;
@property(nonatomic) NSLineBreakMode descriptionLineBreakMode;

@property(nonatomic) UIColor * titleColor;
@property(nonatomic) UIColor * descriptionColor;
@property(nonatomic) UIColor * votesColor;

@property(nonatomic) BOOL showTopBorder;
@property(nonatomic) BOOL showGotoAppIcon;
@property(nonatomic) BOOL showRating;
@property(nonatomic) BOOL showStatusIcon;
@property(nonatomic) BOOL showBubbleIcon;
@property(nonatomic) BOOL showCoins;
@property(nonatomic) BOOL showCrossNotifIcon;

@end
