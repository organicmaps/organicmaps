//
//  MTRGContentWallAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGBaseNativeAdView.h>
#import <MyTargetSDK/MTRGNativePromoBanner.h>

@interface MTRGContentWallAdView : MTRGBaseNativeAdView

@property(nonatomic, readonly) UIImageView *imageView;
@property(nonatomic) UIEdgeInsets imageMargins;

@end
