//
//  MRAdManAdapter.h
//  MRAdMan
//
//  Created by Пучка Илья on 24.01.14.
//  Copyright (c) 2014 MailRu. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "SKStoreProductViewControllerDelegateExtended.h"
#import "MRAdManFullScreenViewController.h"
#import "MRADSectionsData.h"
#import "MRADSection.h"
#import "MRADShowcaseBanner.h"
#import "MRAdCustomParamsProvider.h"

@protocol MRAdManAdapterDelegate;

/** 
 Класс MRAdManAdapter предоставляет приложению возможность получать данные о рекламных объявлениях для их последующего показа и отсылать необходимую статистику (показы/клики). Данные приходят в формате JSON. Адаптер обрабатывает пришедшие данные и на основе их создаёт объект MRADSection. Полученные данные передаются делегату MRAdManAdapter. Приложение самостоятельно показывает баннеры полученных приложений и вызывает в адаптере методы для отправки статистики ([MRAdManAdapter sendClick:], [MRAdManAdapter sendShow:]).
 
 MRAdManAdapter не показывает баннеры, его задача загрузить данные о рекламе, сохранить эти данные в кэше, передать их приложению и при вызове соответствующих методов отсылать статистику на удалённый сервер.
 */
typedef void (^DeviceParamsCompletionBlock) (NSDictionary * params);
@interface MRAdManAdapter : NSObject

/**
 *  Делегат адаптера
 */
@property (nonatomic, weak) id<MRAdManAdapterDelegate> delegate;

/**
 *  Синглтон адаптера
 *
 *  @return Синглтон адаптера
 *  @discussion Для испльзования синглтона необходимо передать ему [MRAdManAdapter initWithSlot:withFormats:withCustomParamsProvider:]
 */
+ (instancetype)sharedInstance;

/**
 *  Инициализирует адаптер с переданным slotId,
 *
 *  @param slotId    идентификтор слота
 *  @param format    формат объявлений
 *  @param customParamsProvider    объект с дополнительными параметрами
 *
 *  @return Инициализированный объект адаптера
 */
- (instancetype)initWithSlot:(NSInteger)slotId withFormat:(NSString *)format withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider;

/**
 *  Инициирует загрузку данных о рекламе
 *
 *  @param useCache Если true, то будут возвращены закешированные данные, если они есть и время их хранения не истекло. Если закешированных данных нет, или useCache false, то данные буду запрошены по сети.
 */
- (void)reload:(BOOL)useCache;

/**
 *  Очищает закешированные данные для переданного слота и форматов объявлений
 *
 *  @param slotId  Идентификатор слота
 *  @param format Формат объявлений
 */
- (void)clearCacheForSlot:(NSInteger)slotId format:(NSString *)format;

/**
 *  Возвращает копию загруженных адаптером данных
 *
 *  @return Копия загруженных адаптером данных
 */
- (MRADSectionsData *)sectionsData;

/**
 *  Инициирует обработку нажатия баннера
 *
 *  @param banner Баннер, который был нажат
 *  @discussion В результате будет отправлена статистика по нажатию баннера и вызван метод делегата adManBannerClicked:withViewControllerToPresent: 
 *  или adManBannerClicked:withURLToOpen: в зависимости от результата обработки нажатия баннера.
 */
- (void)sendClick:(MRADBanner *)banner;

- (void)deviceParams:(DeviceParamsCompletionBlock)completion;

- (void)trackUrl:(NSString *)trackingUrl;

/**
 *  Инициирует отправку статистики по показу баннера
 *
 *  @param banner Показанный баннер
 *  @discussion Если показано несколько баннеров, нужно вызвать этот метод для каждого из них
 */
- (void)sendShow:(MRADBanner *)banner;

/**
 *  Посылает кастомной событие, связанное с баннером
 *
 *  @param event  Название события
 *  @param banner Баннер, вызвавщий событие
 *  @discussion Событие с таким назвванием должно присутствовать [MRADBanner statistics].
 */
- (void)sendEvent:(NSString *)event forBanner:(MRADBanner *)banner;

/**
 *  Устанавливает режим логирования
 *
 *  @param debugMode True для включения логирования в режиме отладки. По умолчанию логирование отключено
 */
- (void)setDebugMode:(BOOL)debugMode;

/**
 *  Если false, то при нажатии на баннер всегда будет вызываться метод делегата adManBannerClicked:withURLToOpen:.
 *  True нужно передавать, если приложение собирается отображать страницы рекламируемых приложений в самом себе, без перехода в браузер.
 */
@property (nonatomic) BOOL handleLinksInApp;

/**
 *  Время хранения закешированных данных
 */
@property (nonatomic) NSTimeInterval cacheTimeout;

#pragma mark - Deprecated methods

/**
 *  Инициализирует адаптер с переданным slotId,
 *
 *  @param slotId    идентификтор слота
 *  @param formats   форматы объявлений
 *  @param customParamsProvider    объект с дополнительными параметрами
 *  @deprecated Используйте метод [MRAdManAdapter initWithSlot:withFormat:withCustomParamsProvider:]
 *
 *  @return Инициализированный объект адаптера
 */
- (instancetype)initWithSlot:(NSInteger)slotId withFormats:(NSArray *)formats withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider __attribute__((deprecated));

/**
 *  Очищает закешированные данные для переданного слота и форматов объявлений
 *
 *  @param slotId  Идентификатор слота
 *  @param formats Форматы объявлений
 *  @deprecated Используйте [MRAdManAdapter clearCacheForSlot:format:]
 */
- (void)clearCacheForSlot:(NSInteger)slotId formats:(NSArray *)formats __attribute__((deprecated));

@end

#pragma mark - MRMyComAdManAdapter

@interface MRMyComAdManAdapter : MRAdManAdapter

@end

#pragma mark - MRAdManAdapterDelegate
/**
 *  Протокол делегата адаптера
 */
@protocol MRAdManAdapterDelegate <NSObject>

@required

/**
 *  Вызывается при успешной загрузке данных (по сети или из кэша)
 *
 *  @param adapter      Адаптер, загрузивший данные
 *  @param sectionsData Загруженный данные
 */
- (void)adManAdapter:(MRAdManAdapter *)adapter didLoadData:(MRADSectionsData *)sectionsData;

/**
 *  Вызвается, если в результате обработки нажатия баннера приложение должно показать модальный диалог
 *
 *  @param banner         Нажатый баннер
 *  @param viewController UIViewController, который необходимо показать пользователю
 *  @discussion Метод может возвращать либо SKProductViewController, либо UIViewController с UIWebView с загруженной страницей, на которую ведёт баннер
 */
- (void)adManBannerClicked:(MRADBanner *)banner withViewControllerToPresent:(UIViewController *)viewController;

/**
 *  Вызывается, если в результате обработки нажатия на баннер приложение должно открыть ссылку
 *
 *  @param banner Нажатый баннер
 *  @param url    Ссылка, которую нужно открыть
 *  @discussion Ссылка может быть как обычная http ссылка, так и ссылка с кастомной урл схемой приложения, которое рекламирует баннер (если урл схема указана)
 */
- (void)adManBannerClicked:(MRADBanner *)banner withURLToOpen:(NSURL *)url;

@optional

/**
 *  Вызывается при неудачной попытке загрузить или обработать данные
 *
 *  @param adapter Адаптер, вызввававший событие
 *  @param error   Полученные при загрузке или обработке данных ошибка
 */
- (void)adManAdapter:(MRAdManAdapter *)adapter didFailWithError:(NSError *)error;

@end

