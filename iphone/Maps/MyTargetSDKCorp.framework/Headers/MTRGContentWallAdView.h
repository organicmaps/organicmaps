//
//  MTRGContentWallAdView.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGBaseNativeAdView.h>
#import <MyTargetSDKCorp/MTRGNativePromoBanner.h>
#import <MyTargetSDKCorp/MTRGNativeImageBanner.h>

@interface MTRGContentWallAdView : MTRGBaseNativeAdView

@property (strong, nonatomic) MTRGNativeImageBanner * imageBanner;

//Изображение
@property (nonatomic, strong, readonly) UIImageView * imageView;

//Отступы
@property (nonatomic) UIEdgeInsets imageMargins;

@end
