//
//  MTRGNewsFeedAdView.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGBaseNativeAdView.h>
#import <MyTargetSDKCorp/MTRGNativeTeaserBanner.h>
#import <MyTargetSDKCorp/MTRGStarsRatingView.h>

@interface MTRGNewsFeedAdView : MTRGBaseNativeAdView

@property (strong, nonatomic) MTRGNativeTeaserBanner * teaserBanner;

//Иконка
@property (nonatomic, strong, readonly) UIImageView * iconImageView;
//Домен
@property (nonatomic, strong, readonly) UILabel * domainLabel;
//Категория и подкатегория
@property (nonatomic, strong, readonly) UILabel * categoryLabel;
//Дисклеймер
@property (nonatomic, strong, readonly) UILabel * disclaimerLabel;
//Звезды рейтинга (только для приложений)
@property (strong, nonatomic, readonly) MTRGStarsRatingView * ratingStarsView;
//Количество голосов
@property (strong, nonatomic, readonly) UILabel * votesLabel;
//Кнока для перехода
@property (strong, nonatomic) UIView * buttonView;
@property (strong, nonatomic) UILabel * buttonToLabel;


//Отступы
@property (nonatomic) UIEdgeInsets titleMargins;
@property (nonatomic) UIEdgeInsets domainMargins;
@property (nonatomic) UIEdgeInsets disclaimerMargins;
@property (nonatomic) UIEdgeInsets iconMargins;
@property (nonatomic) UIEdgeInsets ratingStarsMargins;
@property (nonatomic) UIEdgeInsets votesMargins;
@property (nonatomic) UIEdgeInsets buttonMargins;
@property (nonatomic) UIEdgeInsets buttonCaptionMargins;

@end
