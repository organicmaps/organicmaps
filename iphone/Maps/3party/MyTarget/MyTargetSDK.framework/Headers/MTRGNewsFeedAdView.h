//
//  MTRGNewsFeedAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGStarsRatingLabel.h>

@interface MTRGNewsFeedAdView : UIView

@property(nonatomic) MTRGNativePromoBanner *banner;
@property(nonatomic) UIColor *backgroundColor;
@property(nonatomic, readonly) UILabel *ageRestrictionsLabel;
@property(nonatomic, readonly) UILabel *adLabel;

@property(nonatomic, readonly) UIImageView *iconImageView;
@property(nonatomic, readonly) UILabel *domainLabel;
@property(nonatomic, readonly) UILabel *categoryLabel;
@property(nonatomic, readonly) UILabel *disclaimerLabel;
@property(nonatomic, readonly) MTRGStarsRatingLabel *ratingStarsLabel;
@property(nonatomic, readonly) UILabel *votesLabel;
@property(nonatomic, readonly) UIView *buttonView;
@property(nonatomic, readonly) UILabel *buttonToLabel;
@property(nonatomic, readonly) UILabel *titleLabel;

@property(nonatomic) UIEdgeInsets contentMargins;
@property(nonatomic) UIEdgeInsets adLabelMargins;
@property(nonatomic) UIEdgeInsets ageRestrictionsMargins;
@property(nonatomic) UIEdgeInsets titleMargins;
@property(nonatomic) UIEdgeInsets domainMargins;
@property(nonatomic) UIEdgeInsets disclaimerMargins;
@property(nonatomic) UIEdgeInsets iconMargins;
@property(nonatomic) UIEdgeInsets ratingStarsMargins;
@property(nonatomic) UIEdgeInsets votesMargins;
@property(nonatomic) UIEdgeInsets buttonMargins;
@property(nonatomic) UIEdgeInsets buttonCaptionMargins;
@property(nonatomic) UIEdgeInsets categoryMargins;

- (void)loadImages;

@end
