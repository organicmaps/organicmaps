//
//  MRGSAdman.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 05.11.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MRGServiceParams.h"

@protocol MRGSAdmanDelegate;

/**
 *  Класс для работы с Adman (витрины, банеры и т.д.)
 */
@interface MRGSAdman : NSObject

/** Делегат. */
@property (nonatomic, weak) id<MRGSAdmanDelegate> delegate;
/** Текст для заголовка витрины. */
@property (nonatomic, copy) NSString *showcaseTitle;
/** Текст для кнопки закрытия витрины. */
@property (nonatomic, copy) NSString *showcaseCloseButtonTitle;

/**
 *  Получение объекта, с помощью которого происходит отображение рекламной витрины и баннеров.
 *  Параметры витрины и баннера будут взяты из объекта настроек, который был указан при инициализации MRGS.
 *
 *  @return Экземпляр класса MRGSAdman.
 */
+ (instancetype)sharedInstance;

/**
 *  Получение объекта, с помощью которого происходит отображение рекламной витрины и баннеров.
 *
 *  @param params Настройки для отображения витрины и баннера.
 *
 *  @return Экземпляр класса MRGSAdman
 */
- (instancetype)initWithParams:(MRGSAdmanParams *)params;

/**
 *  Загрузка данных для витрины.
 *  @warning Данные загружаются для текущего пользователя. Если нужно выдать бонус за оффер, то убедитесь что пользователь авторизован (вызван метод - (BOOL)authorizationUserWithId:(NSString*)ref у MRGSUsers).
 */
- (void)loadShowcaseData;

/**
 *  Загрузка данных для полноэкранного баннера.
 */
- (void)loadFullscreenBannerData;

/**
 *  Отображение витрины.
 *
 *  @param view Экземляр класса UIView, поверх которого будет отображена витрина.
 */
- (void)openShowcaseInView:(UIView *)view;

/**
 *  Отображение полноэкранного баннера.
 *
 *  @param viewController Экземпляр класса UIViewController, поверх которого будет отображен баннер.
 */
- (void)openFullscreenBannerInViewController:(UIViewController *)viewController;

/**
 *  Закрытие витрины из кода приложения.
 */
- (void)closeShowcase;

/**
 *  Закрытие полноэкранного баннера из кода приложения.
 */
- (void)closeFullscreenBanner;

/**
 *  Удаляет объект витрины из памяти. 
 *  @warning Если вы вызвали этот метод, то для повторного открытия витрины необходимо предварительно загрузить данные с помощью метода -(void)loadShowcaseData.
 */
- (void)releaseShowcase;

/**
 *  Удаляет объект полноэкранного баннера из памяти.
 *  @warning Если вы вызвали этот метод, то для повторного открытия баннера необходимо предварительно загрузить данные -(void)loadFullscreenBannerData.
 */
- (void)releaseFullscreenBanner;

@end

/** Делегат для класса MRGSAdman. */
@protocol MRGSAdmanDelegate <NSObject>

@required
/**
 *  Метод, который вызывается при успешной загрузке данных для витрины.
 *
 *  @param mrgsAdman        Экземпляр Adman для работы с витриной или баннером.
 *  @param hasNotifications Флаг, говорящий о том, что в витрине есть выделенные банеры. Этот флаг можно использовать для отображения значка оповещения на кнопке открытия витрины (например, восклицательный знак).
 */
- (void)mrgsAdman:(MRGSAdman *)mrgsAdman didReceiveShowcaseDataAndFoundNotifications:(BOOL)hasNotifications;

/**
 *  Метод, который вызывается при успешной загрузке данных для полноэкранного баннера.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 */
- (void)mrgsAdmanDidReceiveFullscreenBannerData:(MRGSAdman *)mrgsAdman;

@optional
/**
 *  Метод, который вызывается при возникновении ошибки загрузки данных для витрины.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 *  @param error     Описание ошибки.
 */
- (void)mrgsAdman:(MRGSAdman *)mrgsAdman didFailToReceiveShowcaseDataWithError:(NSError *)error;

/**
 *  Метод, который вызывается при возникновении ошибки загрузки данных для полноэкранного баннера.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 *  @param error     Описание ошибки.
 */
- (void)mrgsAdman:(MRGSAdman *)mrgsAdman didFailToReceiveFullscreenBannerDataWithError:(NSError *)error;

/**
 *  Метод, который вызывется при закрытии пользователем витрины.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 */
- (void)mrgsAdmanShowcaseClosed:(MRGSAdman *)mrgsAdman;

/**
 *  Метод, который вызывется при закрытии пользователем полноэкранного баннера.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 */
- (void)mrgsAdmanFullscreenBannerClosed:(MRGSAdman *)mrgsAdman;

/**
 *  Метод, который вызывается в случае отсутствия информации для отображения на витрине.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 *  @discussion Вызывается как в случае возникновения ошибки при загрузке данных (при вызове метода -(void)loadShowcaseData), так и в случае получения успешного ответа от сервера об отсутствии данных.
 */
- (void)mrgsAdmanShowcaseHasNoAds:(MRGSAdman *)mrgsAdman;

/**
 *  Метод, который вызывается в случае отсутствия информации для отображения на полноэкранном баннере.
 *
 *  @param mrgsAdman Экземпляр Adman для работы с витриной или баннером.
 *  @discussion Вызывается как в случае возникновения ошибки при загрузке данных (при вызове метода -(void)loadFullscreenBannerData), так и в случае получения успешного ответа от сервера об отсутствии данных.
 */
- (void)mrgsAdmanFullscreenBannerHasNoAds:(MRGSAdman *)mrgsAdman;

@end

#endif