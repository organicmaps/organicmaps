//  $Id: MRGSPromoCodes.h 5674 2014-10-21 11:01:06Z a.grachev $
//
//  MRGSPromoCodes.h
//  MRGServiceFramework
//
//  Created by AKEB on 19.04.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSPromoCodes_
#define MRGServiceFramework_MRGSPromoCodes_

#import <Foundation/Foundation.h>

@protocol MRGSPromoCodesDelegate;

/** Класс MRGSPromoCodes для работы с промо кодами.
 *
 */
@interface MRGSPromoCodes : NSObject

/** Объект для работы с промо кодами.
 *	@return Экземпляр класса MRGSPromoCodes
 */
+ (instancetype)sharedInstance;

/** Делегат MRGSBankDelegate для обратных вызовов */
@property (nonatomic, weak) id<MRGSPromoCodesDelegate> delegate;

/** Генерирует новый промо код на сервере
 * @param level - Уроаень промо кода (от 1 до 200)
 */
- (void)createCode:(int)level;

/** Запросить все промо коды, которые были сгенерированные доля пользователя
 */
- (void)getAllCodes;

#pragma mark - Deprecated methods
/** Экземпляр класса MRGSPromoCodes.
 *	@return Возвращает экземпляр класса MRGSPromoCodes
 *  @deprecated Используйте [MRGSPromoCodes sharedInstance];
 */
+ (MRGSPromoCodes*)singleton DEPRECATED_ATTRIBUTE;

@end

/** Протокол MRGSPromoCodesDelegate. */
@protocol MRGSPromoCodesDelegate<NSObject>

@required

/** метод протокола вызывается при завершении запроса создания нового промо кода
 *  @param promo информация о промо коде
 */
- (void)createPromoCodeSuccessful:(NSDictionary*)promo;

/** метод протокола вызывается при завершении запроса неудачно
 *  @param error NSError
 */
- (void)promoCodeFailed:(NSError*)error;

/** Метод вызывается при получении ответа от сервера.
 *  @param promos Список полученных промо кодов
 */
- (void)getAllPromoCodesSuccessful:(NSArray*)promos;

@end

#endif