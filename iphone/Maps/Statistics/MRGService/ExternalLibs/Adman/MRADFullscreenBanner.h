//
//  MRADFullscreenBanner.h
//  MRAdMan
//
//  Created by Пучка Илья on 24.03.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import "MRADBanner.h"
#import "MRADBannerImage.h"

/*
 Фулскрин баннер
 */
@interface MRADFullscreenBanner : MRADBanner

@property (nonatomic, copy, readonly) MRADBannerImage *portraitImage;
@property (nonatomic, copy, readonly) MRADBannerImage *portraitImageHd;
@property (nonatomic, copy, readonly) MRADBannerImage *landscapeImage;
@property (nonatomic, copy, readonly) MRADBannerImage *landscapeImageHd;

@property (nonatomic, copy, readonly) NSString *closeIcon;
@property (nonatomic, copy, readonly) NSString *closeIconHd;

@property (nonatomic, copy, readonly) NSArray *landscapeImages;
@property (nonatomic, copy, readonly) NSArray *portraitImages;

@property (nonatomic, readonly) BOOL canClose;

- (CGSize)sizeForOrientation:(UIInterfaceOrientation)orientation;
- (CGSize)sizeForOtherOrientation:(UIInterfaceOrientation)orientation;

- (BOOL)isEqual:(id)other;

- (BOOL)isEqualToBanner1:(MRADFullscreenBanner *)banner;

- (NSUInteger)hash;

@end
