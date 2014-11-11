//
//  MRADBanner.h
//  MRAdMan
//
//  Created by Пучка Илья on 24.03.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

/**
 *  Абстрактный класс баннера
 */
@interface MRADBanner : NSObject<NSCoding, NSCopying>

/**
 *  HTML код баннера
 */
@property (nonatomic, readonly, copy) NSString *html;

/**
 *  Идентификатор баннера
 */
@property (nonatomic, readonly, copy) NSNumber *ID;

/**
 *  URL-схема приложения
 */
@property (nonatomic, readonly, copy) NSString *urlScheme;

/**
 *  Статистики баннера.
 */
@property (nonatomic, readonly, copy) NSArray *statistics;

/**
 *  Bundle идентификатор приложения
 */
@property (nonatomic, readonly, copy) NSString *bundleID;

/**
 *  Описание баннера
 *  Поле description заменено на descriptionText для совместимости с новым XCode
 */
@property (nonatomic, readonly, copy) NSString *descriptionText;

/**
 *  Название баннера
 */
@property (nonatomic, readonly, copy) NSString *title;

/**
 *  Tracking ссылка для подсчёта кликов, она же ведёт (через редиректы) на конечную сслыку баннера (на itunes)
 */
@property (nonatomic, readonly, copy) NSString *trackingURL;

/**
 *  Идентификатор приложения в itunes
 */
@property (nonatomic, readonly, copy) NSString *externId;

@property (nonatomic, readonly, copy) NSString *type;
@property (nonatomic, readonly, copy) NSString *mrgsID;
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) CGFloat timeout;
@property (nonatomic, readonly, copy) NSString *link;
@property (nonatomic, copy, readonly) NSDictionary *jsonDict;

/**
 *  Проверяет установлено ли приложение (используя url-схему приложения) и возвращает true, если установлено, false в противном случае
 *
 *  @return Возвращает true, если приложение установлено, false в противном случае
 */
- (BOOL)isAppInstalled;

- (BOOL)isEqualToBanner:(MRADBanner *)banner;

@end
