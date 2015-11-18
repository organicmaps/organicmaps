//
//  MTRGNativeBaseAd.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 18.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGCustomParams.h>

@interface MTRGNativeBaseAd : NSObject

//Если флаг уставновлен, то картинки будут загружены
@property (nonatomic) BOOL autoLoadImages;
//Загрузить банер (будут вызваны методы делегата)
-(void) load;
//Зарегистрировать view
-(void) registerView:(UIView*)view withController:(UIViewController*)controller;
-(void) unregisterView;

//Параметры
//Если флаг установлен в YES, ссылки и app-store будут открываться внутри приложения
@property (nonatomic) BOOL handleLinksInApp;

//Дополнительный параметры настройки запроса
@property (nonatomic, strong, readonly) MTRGCustomParams * customParams;

-(instancetype) initWithSlotId:(NSString*)slotId;

@end
