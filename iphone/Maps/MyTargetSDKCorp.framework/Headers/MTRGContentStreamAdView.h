//
//  MTRGContentStreamAdView.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 05.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGNativePromoBanner.h>
#import <MyTargetSDKCorp/MTRGBaseNativeAdView.h>
#import <MyTargetSDKCorp/MTRGStarsRatingView.h>

@interface MTRGContentStreamAdView : MTRGBaseNativeAdView

@property (strong, nonatomic) MTRGNativePromoBanner * promoBanner;

//Заголовок
@property (nonatomic, strong, readonly) UILabel * titleLabel;
//Заголовок снизу(для промо)
@property (nonatomic, strong, readonly) UILabel * titleBottomLabel;
//Описание
@property (nonatomic, strong, readonly) UILabel * descriptionLabel;
//Иконка
@property (nonatomic, strong, readonly) UIImageView * iconImageView;
//Изображение
@property (nonatomic, strong, readonly) UIImageView * imageView;
//Домен
@property (nonatomic, strong, readonly) UILabel * domainLabel;
//Домен нижний (для промо)
@property (nonatomic, strong, readonly) UILabel * domainBottomLabel;
//Категория и подкатегория
@property (nonatomic, strong, readonly) UILabel * categoryLabel;
//Категория и подкатегория - снизу
@property (nonatomic, strong, readonly) UILabel * categoryBottomLabel;
//Дисклеймер
@property (nonatomic, strong, readonly) UILabel * disclaimerLabel;
//Звезды рейтинга
@property (strong, nonatomic, readonly) MTRGStarsRatingView * ratingStarsView;
//Количество голосов
@property (strong, nonatomic, readonly) UILabel * votesLabel;
//Кнока для перехода
@property (strong, nonatomic) UIView * buttonView;
@property (strong, nonatomic) UILabel * buttonToLabel;


//Отступы
@property (nonatomic) UIEdgeInsets titleMargins;
@property (nonatomic) UIEdgeInsets titleBottomMargins;
@property (nonatomic) UIEdgeInsets domainMargins;
@property (nonatomic) UIEdgeInsets domainBottomMargins;
@property (nonatomic) UIEdgeInsets categoryMargins;
@property (nonatomic) UIEdgeInsets descriptionMargins;
@property (nonatomic) UIEdgeInsets disclaimerMargins;
@property (nonatomic) UIEdgeInsets imageMargins;
@property (nonatomic) UIEdgeInsets iconMargins;
@property (nonatomic) UIEdgeInsets ratingStarsMargins;
@property (nonatomic) UIEdgeInsets votesMargins;
@property (nonatomic) UIEdgeInsets buttonMargins;
@property (nonatomic) UIEdgeInsets buttonCaptionMargins;


@end
