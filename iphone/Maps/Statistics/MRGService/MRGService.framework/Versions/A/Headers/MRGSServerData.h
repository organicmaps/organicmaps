//  $Id: MRGSServerData.h 5676 2014-10-21 11:16:43Z a.grachev $
//
//  MRGSServerData.h
//  MRGServiceFramework
//
//  Created by Yuriy Lisenkov on 17.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSServerData_
#define MRGServiceFramework_MRGSServerData_

#import <Foundation/Foundation.h>
#import "MRGS.h"

@protocol MRGSServerDataDelegate;

/** Класс MRGSServerData. 
 
 Класс отвечает за проверку и получение данных с сервера например начисление денег, новости и т.д.
 
 Пример инициализации и получения данных
 
	//	Инициализирует класс MRGSServerData, устанавливает делегат и запрашивает данные с сервера
	[[MRGSServerData serverDataWithDelegate:delegate] loadData];
 
	//	Инициализирует класс MRGSServerData, устанавливает делегат и запрашивает данные с сервера
	[[MRGSServerData singleton] setDelegate:delegate];
	[[MRGSServerData singleton] loadData];
 
 ответ приходит в метод протокола:
	-(void) loadServerDataDidFinished:(NSDictionary*)serverData;
 
 в serverData приходят:
 money - тип и количество денег, необходимых для начисления пользователю
 news - новости
 */

@interface MRGSServerData : NSObject 

/** Делегат класса.
 */
@property (nonatomic, weak) id<MRGSServerDataDelegate> delegate;

/**
 *  Объект для получения данных от сервера.
 *
 *  @return Экземпляр класса MRGSServerData
 */
+ (instancetype)sharedInstance;

/** Помечаем выданый бонус!
 * @param bonusId id бонуса (приходит от Сервера)
 */
- (void)bonusInformer:(NSString*)bonusId;

/** Считывает данные с сервера */
- (void)loadData;

/** Забираем промо баннеры с сервера */
- (void)loadPromoBanner;

#pragma mark - Derecated methods

/** Singleton класса
 *
 *	@return Возвращает экземпляр класса MRGSServerData, если он был инициализирован до этого
 *  @deprecated Используйте [MRGSServerData sharedInstance];
 */
+ (MRGSServerData*)singleton DEPRECATED_ATTRIBUTE;

/** Возвращает экземпляр класса MRGSServerData и устанавливает делегат
 * @param delegate протокола
 * @return Возвращает экземпляр класса MRGSServerData
 * @deprecated Используйте [MRGSServerData sharedInstance].delegate = delegate;
 */
+ (MRGSServerData*)serverDataWithDelegate:(id<MRGSServerDataDelegate>)delegate DEPRECATED_ATTRIBUTE;

@end

#pragma mark -
#pragma mark Протокол
/**  @name Протокол */

/** Протокол MRGSServerDataDelegate. */
@protocol MRGSServerDataDelegate<NSObject>

@required

/** метод протокола, срабатывает при получении данных с сервера
 *  @param serverData полученные с сервера данные
 */
- (void)loadServerDataDidFinished:(NSDictionary*)serverData;

@optional

/** метод протокола, срабатывает при завершении инициализации библиотеки
 */
- (void)initializationFinish;

/** метод протокола, срабатывает при получении промо баннеров с сервера
 *  
 *  типы: 1 - Акция, 2 - Событие, 3 - Технические работы
 *
 *  @param promoBanners полученные с сервера промо баннеры
 */
- (void)loadPromoBannersDidFinished:(NSDictionary*)promoBanners;

@end

#endif
