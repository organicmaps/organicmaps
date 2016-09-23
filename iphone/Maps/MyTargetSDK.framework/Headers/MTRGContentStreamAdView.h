//
//  MTRGContentStreamAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGBaseNativeAdView.h>
#import <MyTargetSDK/MTRGStarsRatingView.h>

@interface MTRGContentStreamAdView : MTRGBaseNativeAdView

@property(nonatomic, readonly) UILabel *titleLabel;
@property(nonatomic, readonly) UILabel *titleBottomLabel;
@property(nonatomic, readonly) UILabel *descriptionLabel;
@property(nonatomic, readonly) UIImageView *iconImageView;
@property(nonatomic, readonly) UIImageView *imageView;
@property(nonatomic, readonly) UILabel *domainLabel;
@property(nonatomic, readonly) UILabel *domainBottomLabel;
@property(nonatomic, readonly) UILabel *categoryLabel;
@property(nonatomic, readonly) UILabel *categoryBottomLabel;
@property(nonatomic, readonly) UILabel *disclaimerLabel;
@property(nonatomic, readonly) MTRGStarsRatingView *ratingStarsView;
@property(nonatomic, readonly) UILabel *votesLabel;
@property(nonatomic) UIView *buttonView;
@property(nonatomic) UILabel *buttonToLabel;

@property(nonatomic) UIEdgeInsets titleMargins;
@property(nonatomic) UIEdgeInsets titleBottomMargins;
@property(nonatomic) UIEdgeInsets domainMargins;
@property(nonatomic) UIEdgeInsets domainBottomMargins;
@property(nonatomic) UIEdgeInsets categoryMargins;
@property(nonatomic) UIEdgeInsets descriptionMargins;
@property(nonatomic) UIEdgeInsets disclaimerMargins;
@property(nonatomic) UIEdgeInsets imageMargins;
@property(nonatomic) UIEdgeInsets iconMargins;
@property(nonatomic) UIEdgeInsets ratingStarsMargins;
@property(nonatomic) UIEdgeInsets votesMargins;
@property(nonatomic) UIEdgeInsets buttonMargins;
@property(nonatomic) UIEdgeInsets buttonCaptionMargins;


@end
