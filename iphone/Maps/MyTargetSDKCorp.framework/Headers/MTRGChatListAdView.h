//
//  MTRGChatListAdView.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGBaseNativeAdView.h>
#import <MyTargetSDKCorp/MTRGNativeTeaserBanner.h>
#import <MyTargetSDKCorp/MTRGStarsRatingView.h>

@interface MTRGChatListAdView : MTRGBaseNativeAdView

@property (strong, nonatomic) MTRGNativeTeaserBanner * teaserBanner;

//Заголовок
@property (nonatomic, strong, readonly) UILabel * titleLabel;
//Описание
@property (nonatomic, strong, readonly) UILabel * descriptionLabel;
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

//Отступы
@property (nonatomic) UIEdgeInsets titleMargins;
@property (nonatomic) UIEdgeInsets domainMargins;
@property (nonatomic) UIEdgeInsets categoryMargins;
@property (nonatomic) UIEdgeInsets descriptionMargins;
@property (nonatomic) UIEdgeInsets disclaimerMargins;
@property (nonatomic) UIEdgeInsets iconMargins;
@property (nonatomic) UIEdgeInsets ratingStarsMargins;
@property (nonatomic) UIEdgeInsets votesMargins;

@end
