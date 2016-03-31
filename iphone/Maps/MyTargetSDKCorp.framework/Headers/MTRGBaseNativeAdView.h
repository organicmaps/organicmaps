//
//  MTRGBaseNativeAdView.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 03.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>

//Основная логика view для отображения банера (включает в себя сразу и teaser и banner и promo)
@interface MTRGBaseNativeAdView : UIView

//Цвет фона при нажатии
@property UIColor * backgroundColor;

//Задать ширину View (высота будет определена автоматически в зависимости от содержимого)
-(void) setFixedWidth:(CGFloat)width;
//Задать позицию View
-(void) setPosition:(CGPoint)position;
//Вернуть размер представления
-(CGSize) getSize;


//Элементы управления банера
//Текст для возрастных ограничений
@property (nonatomic, strong, readonly) UILabel * ageRestrictionsLabel;
//Текст надпись реклама
@property (nonatomic, strong, readonly) UILabel * adLabel;

//Отступы:
@property (nonatomic) UIEdgeInsets adTitleMargins;
@property (nonatomic) UIEdgeInsets ageRestrictionsMargins;

//Загрузить изображения, если они не были загружены вручную
-(void)loadImages;

@end
