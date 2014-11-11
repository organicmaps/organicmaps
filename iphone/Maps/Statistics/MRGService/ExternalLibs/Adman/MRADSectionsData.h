//
// Created by a_karimov on 30.12.13.
//
// Copyright (c) 2013 AlexKar. All rights reserved.
//


#import <Foundation/Foundation.h>

@class MRADSection;

/**
 *  Класс-контейнер для данных о рекламе.
 */
@interface MRADSectionsData : NSObject <NSCoding, NSCopying>

/**
 *  Массив секций
 *  @see MRADSection
 */
@property (nonatomic, copy, readonly) NSArray *sections;

/**
 *  html, используемый для отображения рекламы в MRAdManView
 */
@property (nonatomic, copy, readonly) NSString *html;

/**
 *  Набор секций в json-формате
 */
@property (nonatomic, copy, readonly) NSString *json;

@property (nonatomic, copy, readonly) NSDictionary *jsonDict;
@property (nonatomic, copy, readonly) NSString *version;

/**
 *  Возвращает флаг, указывающий на наличие баннеров в секции переданного формата
 *
 *  @param format формат секции
 *
 *  @return Если секция переданного формата существует и в ней есть баннеры, то возвращает true, иначе возвращает false
 */
- (BOOL)hasAdsWithFormat:(NSString *)format;

/**
 *  Возвращает флуг, указывающий на наличие каких-либо рекламных баннеров в секциях
 *
 *  @return True, если в какой-либо секции есть баннеры, fale, если секций нет или они все пустые
 */
- (BOOL)hasAds;

/**
 *  Возвращает флаг, указывающий есть ли уведомления в секциях
 *
 *  @return Если в какой-либо секции есть уведомления, то возвращает true, иначе возвращает false
 *  @see MRADSection
 */
- (BOOL)hasNotifications;

/**
 *  Возвращает флаг, указывающий на наличие уведомлений в секции переданного формата
 *
 *  @param format формат секции
 *
 *  @return Если секция переданного формата существует и в ней есть уведомления, то возвращает true, иначе возвращает false
 *  @see MRADSection
 */
- (BOOL)hasNotificationInSectionWithFormat:(NSString *)format;

/**
 *  Возвращает секцию переданного формата или nil, если такой секции нет
 *
 *  @param format Формат секции
 *
 *  @return Секция переданного формата или nil, если такой секции нет
 */
- (MRADSection *)sectionWithFormat:(NSString *)format;

@end
