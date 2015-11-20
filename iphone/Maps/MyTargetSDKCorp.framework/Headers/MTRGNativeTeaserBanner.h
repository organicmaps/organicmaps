//
//  MTRGNativeTeaserBanner.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MyTargetSDKCorp/MTRGImageData.h>
#import <MyTargetSDKCorp/MTRGTypes.h>


@interface MTRGNativeTeaserBanner : NSObject


// Текст "Реклама"
@property (nonatomic, strong) NSString * advertisingLabel;
// Текст возрастного ограничения. Пример: "18+"
@property (nonatomic, strong) NSString * ageRestrictions;
// Тип навигации (веб, магазин приложений).
@property (nonatomic) MTRGNavigationType navigationType;
// Текст заголовка банера
@property (nonatomic, strong) NSString * title;
// Текст описания банера
@property (nonatomic, strong) NSString * descriptionText;
// Текст рекламного предупреждения
@property (nonatomic, strong) NSString * disclaimer;
// Рейтинг рекламируемого приложения в диапазоне [0.0, 5.0]
@property (nonatomic, strong) NSNumber * rating;
// Количество голосов, принимавших участие в рейтинге рекламируемого приложения.
@property (nonatomic, strong) NSNumber * votes;
// Домен рекламируемого веб-ресурса.
@property (nonatomic, strong) NSString * domain;
// Текст Call To Action - призыв к действию. Пример: "Установить"
@property (nonatomic, strong) NSString * ctaText;
// Название категории рекламируемого приложения.
@property (nonatomic, strong) NSString * category;
// Название подкатегории рекламируемого приложения.
@property (nonatomic, strong) NSString * subcategory;

//Иконка
@property (nonatomic,strong) MTRGImageData * icon;

@end
