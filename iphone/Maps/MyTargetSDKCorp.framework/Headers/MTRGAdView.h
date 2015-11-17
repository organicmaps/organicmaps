//
//  MTRGAdView.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 05.03.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGCustomParams.h>

@class MTRGAdView;

@protocol MTRGAdViewDelegate <NSObject>

-(void)onLoadWithAdView:(MTRGAdView *)adView;
-(void)onNoAdWithReason:(NSString *)reason adView:(MTRGAdView *)adView;

@optional

-(void)onAdClickWithAdView:(MTRGAdView *)adView;

@end


@interface MTRGAdView : UIView

-(instancetype) initWithSlotId:(NSString*)slotId;

-(instancetype) initWithSlotId:(NSString*)slotId withRefreshAd:(BOOL)refreshAd;

//Загрузить банер
-(void) load;

-(void) start;
-(void) stop;

@property (nonatomic, weak) id<MTRGAdViewDelegate> delegate;
//Дополнительный параметры настройки запроса
@property (nonatomic, strong, readonly) MTRGCustomParams * customParams;
//Если флаг установлен в YES, ссылки и app-store будут открываться внутри приложения
@property (nonatomic) BOOL handleLinksInApp;
//Контроллер, используется в режиме handleLinksInApp = YES
@property (nonatomic, weak) UIViewController * viewController;


@end
