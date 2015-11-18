//
//  MTRGNativeAppwallAd.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 13.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <MyTargetSDKCorp/MTRGCustomParams.h>
#import <MyTargetSDKCorp/MTRGNativeAppwallBanner.h>
#import <MyTargetSDKCorp/MTRGAppwallAdView.h>


@class MTRGNativeAppwallAd;

@protocol MTRGNativeAppwallAdDelegate <NSObject>

-(void)onLoadWithAppwallBanners:(NSArray *)appwallBanners appwallAd:(MTRGNativeAppwallAd *)appwallAd;
-(void)onNoAdWithReason:(NSString *)reason appwallAd:(MTRGNativeAppwallAd *)appwallAd;

@optional

-(void)onAdClickWithNativeAppwallAd:(MTRGNativeAppwallAd *)appwallAd appwallBanner:(MTRGNativeAppwallBanner *)appwallBanner;

@end




@interface MTRGNativeAppwallAd : NSObject


@property (weak, nonatomic) id<MTRGNativeAppwallAdDelegate> delegate;

//Загрузить банер (будут вызваны методы делегата)
-(void) load;

//Параметры
//Если флаг установлен в YES, ссылки и app-store будут открываться внутри приложения
@property (nonatomic) BOOL handleLinksInApp;
//Название витрины
@property (copy, nonatomic) NSString * appWallTitle;
//Название кнопки закрытие
@property (copy, nonatomic) NSString * closeButtonTitle;
//Период кэширования баннеров
@property (nonatomic) NSUInteger cachePeriodInSec;


//Дополнительный параметры настройки запроса
@property (nonatomic, strong, readonly) MTRGCustomParams * customParams;

-(instancetype) initWithSlotId:(NSString*)slotId;

//Отображение:
-(void) showWithController:(UIViewController*) controller onComplete:(void(^)()) onComplete
                        onError:(void(^)(NSError* error))onError;


//Зарегистрировать для отображения вьюшку
-(void) registerAppWallAdView:(MTRGAppwallAdView *)appWallAdView withController:(UIViewController*)controller;

//Сообщить о том, что бынер был показан
-(void) handleShow:(MTRGNativeAppwallBanner *)appWallBanner;
//Сообщить о том, что по банеру был произведен клик
-(void) handleClick:(MTRGNativeAppwallBanner *)appWallBanner withController:(UIViewController*)controller;
//Сообщить о наличии нотификаций
-(BOOL) hasNotifications;

@property (nonatomic, strong, readonly) NSArray * banners;

-(void) close;

@end
