//
//  MTRGNativeAppwallBanner.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 13.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGImageData.h>


@interface MTRGNativeAppwallBanner : NSObject


//Статус банера (шильдик)
@property (nonatomic, copy) NSString * status;
//Изображение шильдика
@property (nonatomic,strong) MTRGImageData * statusImage;
//Наличие нотификации у банера
@property (nonatomic) BOOL hasNotification;
//Текст заголовка
@property (nonatomic, copy) NSString * title;
//Текст описания
@property (nonatomic, copy) NSString * descriptionText;

//Иконка
@property (nonatomic,strong) MTRGImageData * icon;
//рейтинг приложения в сторе, неотрицательное число
@property (nonatomic, copy) NSNumber * rating;
// количество оценок в сторе, неотрицательное число
@property (nonatomic, copy) NSNumber * votes;
//Валюта в офере
//Коичество валюты
@property (nonatomic, copy) NSNumber * coins;
//Цвет фона плашки
@property (nonatomic, copy) UIColor * coinsBgColor;
//Цвет текста количества
@property (nonatomic, copy) UIColor * coinsTextColor;
//Изображение валюты
@property (nonatomic,strong) MTRGImageData * coinsIcon;

@property (strong, nonatomic) MTRGImageData * bubbleIcon;
@property (strong, nonatomic) MTRGImageData * gotoAppIcon;
@property (strong, nonatomic) MTRGImageData * itemHighlightIcon;

//Установлено ли приложение, метод необходим только в том случае, если рекламируется приложение.
-(BOOL) isAppInstalled;


//Поля, используемые только Corp-разработчиками
@property (nonatomic, copy) NSString * mrgsId;

//длительность показа баннера (не используется для данного типа), число
@property (nonatomic, copy) NSNumber * timeout;
// только для почты. Звездочка в списке писем. пока не используется, булево значение
@property (nonatomic) BOOL main;
// только для почты. Выделяет заголовок списка, в котором находится баннер.
@property (nonatomic) BOOL requireCategoryHighlight;
// только для почты. не используется. булево значение
@property (nonatomic) BOOL banner;
// только для почты. Показывать баннер в витрине только при wi-fi соединении. булево значение.
@property (nonatomic) BOOL requireWifi;
// тип приложения (платное/бесплатное), строка, может быть пустой
@property (nonatomic, copy) NSString * paidType;




@end
