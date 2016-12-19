//
//  MTRGNewsFeedAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGBaseNativeAdView.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGStarsRatingView.h>

@interface MTRGNewsFeedAdView : MTRGBaseNativeAdView

@property(nonatomic, readonly) UIImageView *iconImageView;
@property(nonatomic, readonly) UILabel *domainLabel;
@property(nonatomic, readonly) UILabel *categoryLabel;
@property(nonatomic, readonly) UILabel *disclaimerLabel;
@property(nonatomic, readonly) MTRGStarsRatingView *ratingStarsView;
@property(nonatomic, readonly) UILabel *votesLabel;
@property(nonatomic, readonly) UIView *buttonView;
@property(nonatomic, readonly) UILabel *buttonToLabel;
@property(nonatomic, readonly) UILabel *titleLabel;

@property(nonatomic) UIEdgeInsets titleMargins;
@property(nonatomic) UIEdgeInsets domainMargins;
@property(nonatomic) UIEdgeInsets disclaimerMargins;
@property(nonatomic) UIEdgeInsets iconMargins;
@property(nonatomic) UIEdgeInsets ratingStarsMargins;
@property(nonatomic) UIEdgeInsets votesMargins;
@property(nonatomic) UIEdgeInsets buttonMargins;
@property(nonatomic) UIEdgeInsets buttonCaptionMargins;
@property(nonatomic) UIEdgeInsets categoryMargins;

@end
