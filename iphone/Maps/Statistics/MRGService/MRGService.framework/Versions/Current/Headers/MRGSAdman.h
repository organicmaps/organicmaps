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
 *
 *  @return Экземпляр класса MRGSAdman.
 */
+ (instancetype)sharedInstance;

/**
 *  Загрузка данных для витрины.
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
 *  @param hasNotifications Флаг, говорящий о том, что в витрине есть выделенные банеры. Этот флаг можно использовать для отображения значка оповещения на кнопке открытия витрины (например, восклицательный знак).
 */
- (void)mrgsAdmanDidReceiveShowcaseDataAndFoundNotifications:(BOOL)hasNotifications;

/**
 *  Метод, который вызывается при успешной загрузке данных для полноэкранного баннера.
 */
- (void)mrgsAdmanDidReceiveFullscreenBannerData;

@optional
/**
 *  Метод, который вызывается при возникновении ошибки загрузки данных для витрины.
 *
 *  @param error Описание ошибки.
 */
- (void)mrgsAdmanDidFailToReceiveShowcaseDataWithError:(NSError *)error;

/**
 *  Метод, который вызывается при возникновении ошибки загрузки данных для полноэкранного баннера.
 *
 *  @param error Описание ошибки.
 */
- (void)mrgsAdmanDidFailToReceiveFullscreenBannerDataWithError:(NSError *)error;

/**
 *  Метод, который вызывется при закрытии пользователем витрины.
 */
- (void)mrgsAdmanShowcaseClosed;

/**
 *  Метод, который вызывется при закрытии пользователем полноэкранного баннера.
 */

- (void)mrgsAdmanFullscreenBannerClosed;

/**
 *  Метод, который вызывается при успешной загрузке данных для полноэкранного баннера.
 *
 *  @param hasNotifications  Флаг наличия новых данных с момента последнего просмотра.
 */
- (void)mrgsAdmanDidReceiveFullscreenBannerDataAndFoundNotifications:(BOOL)hasNotifications DEPRECATED_ATTRIBUTE;

@end

#endif