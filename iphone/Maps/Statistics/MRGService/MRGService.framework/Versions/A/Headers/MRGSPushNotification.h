//  $Id: MRGSPushNotification.h 5667 2014-10-20 15:15:54Z a.grachev $
//
//  MRGSPushNotication.h
//  MRGServiceFramework
//
//  Created by AKEB on 09.06.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/** MRGSPushNotication - класс для локальных пуш нотификаций. Экземпляр пуш нотификации.
 *  @deprecated Используйте класс MRGSLocalNotification
 */
DEPRECATED_ATTRIBUTE
@interface MRGSPushNotification : NSObject {
@private
    NSString* _title;
    NSString* _sound;
    NSString* _ref;
    NSDate* _date;
    int _unixTimeStamp;
    int _badgeNumber;
    NSDictionary* _data;
}

/** Текст локальной пуш нотификации */
@property (nonatomic, copy) NSString* title;

/** Звук локальной пуш нотификации */
@property (nonatomic, copy) NSString* sound;

/** Идентификатор локальной пуш нотификации */
@property (nonatomic, copy) NSString* ref;

/** Время локальной пуш нотификации (NSDate) */
@property (nonatomic, copy) NSDate* date;

/** Время локальной пуш нотификации (int) */
@property (nonatomic) int unixTimeStamp;

/** Цифра на иконке */
@property (nonatomic) int badgeNumber;

/** Дополнительные параметры локальной пуш нотификации */
@property (nonatomic, strong) NSDictionary* data;

/** Создание стандартной пуш нотификации. Звук по умолчанию, номер на иконке - 1
 * @param title Текст пуш нотификации
 * @param ref Идентификатор пуш нотификации
 * @param date Дата и время пуш нотификации
 * @return Экземпляр класса MRGSPushNotification
 * @see init initWithTitle:andRef:andDate:
 */
+ (MRGSPushNotification*)pushNotificationWithTitle:(NSString*)title andRef:(NSString*)ref andDate:(NSDate*)date;

/** Описание экземпляра класса
 * @return Строка описание объекта
 */
- (NSString*)description;

/** Создание стандартной пуш нотификации. Звук по умолчанию, номер на иконке - 1
 * @param title Текст пуш нотификации
 * @param ref Идентификатор пуш нотификации
 * @param date Дата и время пуш нотификации
 * @return Экземпляр класса MRGSPushNotification
 * @see init pushNotificationWithTitle:andRef:andDate:
 */
- (id)initWithTitle:(NSString*)title andRef:(NSString*)ref andDate:(NSDate*)date;

@end
