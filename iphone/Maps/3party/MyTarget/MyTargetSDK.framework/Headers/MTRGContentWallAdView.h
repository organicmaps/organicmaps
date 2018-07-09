//
//  MTRGContentWallAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>
#import <MyTargetSDK/MTRGMediaAdView.h>

@interface MTRGContentWallAdView : UIView

@property(nonatomic) MTRGNativePromoBanner *banner;
@property(nonatomic) UIColor *backgroundColor;

@property(nonatomic, readonly) UILabel *ageRestrictionsLabel;
@property(nonatomic, readonly) UILabel *adLabel;

@property(nonatomic) UIEdgeInsets contentMargins;
@property(nonatomic) UIEdgeInsets adLabelMargins;
@property(nonatomic) UIEdgeInsets ageRestrictionsMargins;

@property(nonatomic, readonly) MTRGMediaAdView *mediaAdView;
@property(nonatomic) UIEdgeInsets imageMargins;

- (void)loadImages;

@end
