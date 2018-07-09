//
//  MTRGContentStreamAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGStarsRatingLabel.h>
#import <MyTargetSDK/MTRGMediaAdView.h>

@class MTRGPromoCardCollectionView;

@interface MTRGContentStreamAdView : UIView

@property(nonatomic) MTRGNativePromoBanner *banner;
@property(nonatomic) UIColor *backgroundColor;
@property(nonatomic, readonly) UILabel *ageRestrictionsLabel;
@property(nonatomic, readonly) UILabel *adLabel;

@property(nonatomic, readonly) UILabel *titleLabel;
@property(nonatomic, readonly) UILabel *titleBottomLabel;
@property(nonatomic, readonly) UILabel *descriptionLabel;
@property(nonatomic, readonly) UIImageView *iconImageView;
@property(nonatomic, readonly) MTRGMediaAdView *mediaAdView;
@property(nonatomic, readonly) MTRGPromoCardCollectionView *cardCollectionView;
@property(nonatomic, readonly) UILabel *domainLabel;
@property(nonatomic, readonly) UILabel *domainBottomLabel;
@property(nonatomic, readonly) UILabel *categoryLabel;
@property(nonatomic, readonly) UILabel *categoryBottomLabel;
@property(nonatomic, readonly) UILabel *disclaimerLabel;
@property(nonatomic, readonly) MTRGStarsRatingLabel *ratingStarsLabel;
@property(nonatomic, readonly) UILabel *votesLabel;
@property(nonatomic, readonly) UIView *buttonView;
@property(nonatomic, readonly) UILabel *buttonToLabel;

@property(nonatomic) UIEdgeInsets contentMargins;
@property(nonatomic) UIEdgeInsets adLabelMargins;
@property(nonatomic) UIEdgeInsets ageRestrictionsMargins;
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

- (void)loadImages;

@end
