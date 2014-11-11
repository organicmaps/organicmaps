//  $Id: MRGSMoreGames.h 5656 2014-10-20 10:48:36Z a.grachev $
//
//  MRGSMoreGames.h
//  MRGServiceFramework
//
//  Created by AKEB on 23.04.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSMoreGames_
#define MRGServiceFramework_MRGSMoreGames_

#import <Foundation/Foundation.h>
#import "MRGS.h"

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)

@class MRMyComAdManView, MRADSectionsData;
@protocol MRGSMoreGamesDelegate;

/**  Класс MRGSMoreGames. В инициализации не нуждается. Существовать должен только 1 экземпляр класса
 *
 */
@interface MRGSMoreGames : NSObject<UIWebViewDelegate>

#pragma mark -
#pragma mark ПАРАМЕТРЫ
/**  @name ПАРАМЕТРЫ */

/** Делегат класса.
 */
@property (nonatomic, weak) id<MRGSMoreGamesDelegate> delegate;

/** Заголовок окна */
@property (nonatomic, copy) NSString* title;

/** Название кнопки назад */
@property (nonatomic, copy) NSString* backButtonTitle;

/** Бабл на иконке для витрины */
@property (readonly, nonatomic) BOOL notification;

/** Витрина загружена и готова */
@property (readonly, nonatomic, getter=isReady) BOOL ready;

/** Витрина Открыта */
@property (readonly, nonatomic, getter=isOpened) BOOL opened;

/** Витрина игр и приложений AdMan */
@property (readonly, strong, nonatomic) MRMyComAdManView* adManView;

/** Флвг для опеределения необходимости завершения отложенной инициализации AdMan. Значение по умолчанию - NO.
 *  @discussion Если значение YES, то для завершения инициализации AdMan следует вызвать - (void)completeAdManLazyInit;
 */
@property (nonatomic) BOOL shouldCompleteAdManLazyInit;

/**
 *  Получение объекта, с помощью которого происходит отображение рекламной витрины.
 *
 *  @return Экземпляр класса MRGSMoreGames.
 */
+ (instancetype)sharedInstance;

/** Открывает раздел еще игры
 *	@param rootview UIView на котором будет отображаться раздел
 */
- (void)open:(UIView*)rootview;

/** Открывает раздел еще игры
 * @param viewController UIViewController на котором будет отображаться раздел
 */
- (void)openWithViewController:(UIViewController*)viewController;

/**
 *   Обновление баннеров Витрины
 */
- (void)reload;

/**
 *  Закрывает окно витрины
 */
- (void)close;

/** Завершение отложенной инициализации AdMan.
 *  @discussion Выполняется только в случае, если значение shoulCompleteAdManLazyInit равно YES;
 */
- (void)completeAdManLazyInit;

#pragma mark - Deprecated methods and delegates

/** Экземпляр класса MRGSMoreGames.
 *	@return Возвращает экземпляр класса MRGSMoreGames
 *  @deprecated Используйте метод [MRGSMoreGames sharedInstance]
 */
+ (MRGSMoreGames*)singleton DEPRECATED_ATTRIBUTE;

/** Открывает раздел еще игры
 * @param rootview UIView на котором будет отображаться раздел
 * @param catalog int флаг каталога
 */
- (void)open:(UIView*)rootview andCatalog:(int)catalog DEPRECATED_ATTRIBUTE;

@end

#endif

/** Протокол MRGSMoreGamesDelegate. */
@protocol MRGSMoreGamesDelegate<NSObject>

@required

/** метод протокола, срабатывает при получении данных о баннерах для Витрины
 *  @param notification Если True, то нужно показать бабл на кнопке
 */
- (void)loadBannersDidFinished:(BOOL)notification;

@optional

/**
 *   Метод вызывается перед перезагрузкой данных витрины
 */
- (void)willLoadBanners;

/** метод протокола, срабатывает при возникновении ошибки получения данных о баннерах для Витрины
 *  @param error Описание ошибки
 */
- (void)loadBannersDidFailWithError:(NSError*)error;

/**
 *   Метод вызывается при закрытии витрины
 */
- (void)bannersViewDidClosed;

@end

#endif