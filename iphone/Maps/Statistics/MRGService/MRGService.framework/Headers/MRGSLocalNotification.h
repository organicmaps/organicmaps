//
//  MRGSLocalNotification.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 14.10.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Класс для создания локального уведомления. */
@interface MRGSLocalNotification : NSObject
/** Текст локального уведомления. */
@property (nonatomic, copy) NSString *title;
/** Звук локального уведомления.
 *  Значение по умолчанию: 
 *  iOS - UILocalNotificationDefaultSoundNam
 *  OSX - NSUserNotificationDefaultSoundName
 */
@property (nonatomic, copy) NSString *sound;
/** Идентификатор локального уведомления. */
@property (nonatomic, copy) NSString *identifier;
/** Время появления локального уведомления. */
@property (nonatomic, copy) NSDate *date;
/** Цифра на иконке. */
@property (nonatomic) NSInteger badgeNumber;
/** Дополнительные параметры локального уведомления. */
@property (nonatomic, strong) NSDictionary *data;

/**
 *  Создание локального уведомления. Звук по умолчанию, номер на иконке - 1.
 *
 *  @param title        Текст локального уведомления.
 *  @param identifier   Идентификатор локального уведомления.
 *  @param date         Дата появления локального уведомления.
 *
 *  @return кземпляр класса MRGSLocalNotification
 */
- (instancetype)initWithTitle:(NSString *)title
                   identifier:(NSString *)identifier
                         date:(NSDate *)date;

@end
