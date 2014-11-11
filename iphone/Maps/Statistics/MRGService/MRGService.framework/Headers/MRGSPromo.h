//  $Id: MRGSPromo.h 5731 2014-10-23 10:05:08Z a.grachev $
//
//  MRGSPromo.h
//  MRGServiceFramework
//
//  Created by AKEB on 25.07.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSPromo_
#define MRGServiceFramework_MRGSPromo_

#import <Foundation/Foundation.h>
#import "MRGS.h"

@protocol MRGSPromoDelegate;

/**  Класс MRGSPromo
 *
 */
DEPRECATED_ATTRIBUTE
@interface MRGSPromo : NSObject 

#pragma mark -
#pragma mark МЕТОДЫ КЛАССА
/**  @name МЕТОДЫ КЛАССА */

/**
 * получает информации о рекламируемых товарах
 */
+ (void)getPromo;

/**
 * событие необходимо вызвать, когда баннеры показаны
 * @param appIds список баннеров (bannerID)
 */
+ (void)promoShow:(NSArray*)appIds;

/**
 * событие необходимо вызвать, когда баннер был нажат
 * @param appId баннера который был нажат  (bannerID)
 */
+ (void)promoClick:(int)appId;

/**
 * Отправка любого события в RB
 * @param action код события
 * @param appIds список баннеров (bannerID)
 */
+ (void)evenyNotifierWithAction:(NSString*)action andAppIds:(NSArray*)appIds;

@end

#pragma mark -
#pragma mark Протокол
/**  @name Протокол */

/** Протокол MRGSPromoDelegate. */

@protocol MRGSPromoDelegate<NSObject>

@required
/** метод протокола, срабатывает при получении данных с сервера
 *  @param jsonData полученные с сервера данные
 */
- (void)MRGSPromoLoadDataSuccess:(NSString*)jsonData;

@optional
/** метод протокола, срабатывает при ошибке
 *  @param error ошибка
 */
- (void)MRGSPromoLoadDataFail:(NSError*)error;

@end

#endif
