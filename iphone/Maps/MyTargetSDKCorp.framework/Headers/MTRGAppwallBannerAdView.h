//
//  MTRGAppwallBannerAdView.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 15.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGNativeAppwallBanner.h>

@interface MTRGAppwallBannerAdView : UIView

@property (strong, nonatomic) MTRGNativeAppwallBanner * appWallBanner;


//Цвет фона при нажатии
@property UIColor * activeBackgroundColor;
//Задать ширину View (высота будет определена автоматически в зависимости от содержимого)
-(void) setFixedWidth:(CGFloat)width;
//Задать позицию View
-(void) setPosition:(CGPoint)position;
//Вернуть размер представления
-(CGSize) getSize;

//Заголовок
@property (nonatomic, strong, readonly) UILabel * titleLabel;
//Описание
@property (nonatomic, strong, readonly) UILabel * descriptionLabel;
//Иконка
@property (nonatomic, strong, readonly) UIImageView * iconImageView;


-(instancetype) initWithBanner:(MTRGNativeAppwallBanner *)appWallBanner;

//Подписаться на событие клика
-(void) setOnClick:(void(^)(MTRGAppwallBannerAdView * appWallBannerView))onClick;

@end
