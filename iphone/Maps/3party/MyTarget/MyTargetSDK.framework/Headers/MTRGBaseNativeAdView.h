//
//  MTRGBaseNativeAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 03.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>

@interface MTRGBaseNativeAdView : UIView

@property(nonatomic) MTRGNativePromoBanner *banner;
@property(nonatomic) UIColor *backgroundColor;
@property(nonatomic, readonly) UILabel *ageRestrictionsLabel;
@property(nonatomic, readonly) UILabel *adLabel;
@property(nonatomic) UIEdgeInsets adLabelMargins;
@property(nonatomic) UIEdgeInsets ageRestrictionsMargins;

- (void)setFixedWidth:(CGFloat)width;

- (void)setPosition:(CGPoint)position;

- (CGSize)getSize;

- (void)loadImages;

@end
