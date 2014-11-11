//
//  MRGSNotificationManager.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 14.10.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRGSLocalNotification;

/** Класс для управления уведомлениями. */
@interface MRGSNotificationManager : NSObject
/**
*  Возвращает объект для управления уведомлениями.
*
*  @return Экземпляр класса MRGSNotificationManager.
*/
+ (instancetype)sharedInstance;

/**
 *  Добавление локального уведомления в очередь.
 *
 *  @param notification Локальное уведомление.
 */
- (void)scheduleLocalNotification:(MRGSLocalNotification *)notification;

/**
 *  Поиск локального уведомления в очереди.
 *
 *  @param identifier Идентификатор локального уведомления.
 */
- (MRGSLocalNotification *)findLocalNotificationWithIdentifier:(NSString *)identifier;

/**
 *  Получение списка всех локальных уведомлений в очереди.
 *
 *  @return Список локальных уведомлений.
 */
- (NSArray *)allLocalNotifications;

/**
 *  Удаление локального уведомления из очереди.
 *
 *  @param identifier Идентификатор локального уведомления.
 */
- (void)cancelLocalNotificationWithIdentifier:(NSString *)identifier;

/**
 *  Удаление всех локальных уведомлений в очереди.
 */
- (void)cancelAllLocalNotifications;

@end
