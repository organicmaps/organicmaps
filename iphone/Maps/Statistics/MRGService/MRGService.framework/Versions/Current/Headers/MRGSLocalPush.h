//  $Id: MRGSLocalPush.h 5604 2014-10-14 10:59:41Z a.grachev $
//
//  MRGSLocalPush.h
//  MRGServiceFramework
//
//  Created by AKEB on 09.06.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSLocalPush_
#define MRGServiceFramework_MRGSLocalPush_

#import <Foundation/Foundation.h>
#import "MRGS.h"


@class MRGSPushNotification;

/**  Класс MRGSLocalPush. В инициализации не нуждается. Существовать должен только 1 экземпляр класса
 *  @deprecated Используйте класс MRGSNotificationManager
 */
DEPRECATED_ATTRIBUTE
@interface MRGSLocalPush : NSObject {
}

#pragma mark -
#pragma mark ПАРАМЕТРЫ
/**  @name ПАРАМЕТРЫ */

#pragma mark -
#pragma mark МЕТОДЫ КЛАССА
/**  @name МЕТОДЫ КЛАССА */

/** Экземпляр класса MRGSLocalPush.
 *	@return Возвращает экземпляр класса MRGSLocalPush
 */
+ (MRGSLocalPush*)singleton;

#pragma mark -
#pragma mark МЕТОДЫ ЭКЗЕМПЛЯРА
/**  @name МЕТОДЫ ЭКЗЕМПЛЯРА */

/** Добавить локальный пуш в очередь
 *	@param notification Экземпляр класса MRGSPushNotification
 */
- (void)addLocalPush:(MRGSPushNotification*)notification;

/** Удалить локальный пуш из очереди
 *	@param ref Идентификатор локального пуша
 */
- (void)removeLocalPush:(NSString*)ref;

/** Возвращает локальный пуш
 *	@param ref Идентификатор локального пуша
 * @return Экземпляр класса MRGSPushNotification
 */
- (MRGSPushNotification*)getLocalPush:(NSString*)ref;

/** Возвращает все локальные пуши
 *	@return Массив с экземплярами классов MRGSPushNotification
 */
- (NSArray*)getAllLocalPushes;

/** Удалить все локальные пуш нотификации из очереди */
- (void)clearAllLocalPushes;

@end
#endif