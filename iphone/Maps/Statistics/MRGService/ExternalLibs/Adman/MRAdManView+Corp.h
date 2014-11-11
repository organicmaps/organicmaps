//
//  MRAdManView+Corp.h
//  MRAdMan
//
//  Created by Пучка Илья on 07.03.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import "MRAdManView.h"
#import "MRADSectionsData.h"
#import "MRADSection.h"
#import "MRADBanner.h"

@class MRAdCustomParamsProvider;

@interface MRAdManView ()

/**
 *  Заголовок витрины
 */
@property (nonatomic, copy) NSString *fullScreenTitle;

/**
 *  Текст кнопки закрытия витрины
 */
@property (nonatomic, copy) NSString *fullScreenCloseButtonTitle;

/**
 *  Если true, то при показе витрины status bar будет скрываться (только если витрина показана в модальном окне)
 *  @see [MRAdManView startWithFormat:fullscreenInViewContoller:animated:]
 */
@property (nonatomic) BOOL hideStatusBarInFullScreen;

/**
 *  UINavigationBar, который отображается в полноэкранном режиме.
 *  @discussion Заголовок этого UINavigationBar устанавливается равным [MRAdManView fullScreenTitle], а  заголовок кнопки закрытия устанавливается раным [MRAdManView fullScreenCloseButtonTitle]
 */
@property (nonatomic, strong) UINavigationBar *navigationBar;

/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param format формат секций
 *  @param customParamsProvider  объект с дополнительными параметрами
 *  @discussion Загружает данные о рекламе по сети
 */
- (void)loadSlot:(NSUInteger)slotId
      withFormat:(NSString *)format
    withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider;

/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param format формат секций
 *  @param customParamsProvider  объект с дополнительными параметрами
 *  @param ignoreCache если true, то закешированные данные будут проигнорированы
 *  @discussion Загружает данные о рекламе по сети
 */
- (void)loadSlot:(NSUInteger)slotId
      withFormat:(NSString *)format
    withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider
     ignoreCache:(BOOL)ignoreCache;


/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param padId   идентификатор площадки
 *  @param format формат секций
 *  @param customParamsProvider  объект с дополнительными параметрами
 *  @param ignoreCache если true, то закешированные данные будут проигнорированы
 *  @discussion Загружает данные о рекламе по сети
 */
- (void)loadSlot:(NSUInteger)slotId
       withPadId:(NSString *)padId
      withFormat:(NSString *)format
withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider
     ignoreCache:(BOOL)ignoreCache;


/**
 *  Инициализирует адман с ранее загруженными данными
 *
 *  @param sectionsData данные о рекламе
 *  @discussion прокидывает данные в UIWebView (html + bannersJSON)
 */
- (void)initWithSectionsData:(MRADSectionsData *)sectionsData;

/**
 *  Обновляет данные о рекламе
 *
 *  @param sectionsData  данные о рекламе
 *  @discussion прокидывает новые данные в UIWebView
 */
- (void)updateSectionsData:(MRADSectionsData *)sectionsData;

/**
 *  Загруженные данные о рекламе
 *
 *  @return загруженные данные о рекламе
 */
- (MRADSectionsData *)sectionsData;

/**
 *  Очищает закешированные данные для переданного слота и форматов объявлений
 *
 *  @param slotId  Идентификатор слота
 *  @param padId   идентификатор площадки
 *  @param format  Формат объявлений
 *  @deprecated Используйте метод [MRAdManView clearCacheForSlot:format:]
 */
- (void)clearCacheForSlot:(NSInteger)slotId padId:(NSString *)padId format:(NSString *)format;

/**
 *  Начинает показ рекламы определённого формата в переданном view
 *
 *  @param format     формат секции
 *  @param containerView view, в котором будет показываться реклама
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров в секции с переданным форматом.
 */
- (BOOL)startWithFormat:(NSString *)format inView:(UIView *)containerView;

/**
 *  Начинает показ рекламы определённого формата в переданном view
 *
 *  @param format     формат рекламы
 *  @param excludeBanners массив баннеров (MRAdBanner), которые необходимо исключить из показа
 *  @param containerView view, в котором будет показываться реклама
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров в секции с переданным форматом.
 *  @discussion  Если исключить из показа все баннеры секции, то метод вернёт False, а реклама не будет показана.
 */
- (BOOL)startWithFormat:(NSString *)format exludeBanners:(NSArray *)excludeBanners inView:(UIView *)containerView;

/**
 *  Начинает показ рекламы определённого формата в модальном окне
 *
 *  @param format               формат секции
 *  @param containerViewController view controller, из которого будет открыто модальное окно с рекламой
 *  @param animated             True, чтобы открывать модальное окно анимаированно
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров в секции с переданным форматом.
 */
- (BOOL)startWithFormat:(NSString *)format fullscreenInViewContoller:(UIViewController *)containerViewController animated:(BOOL)animated;

/**
 *  Начинает показ рекламы определённого формата в модальном окне
 *
 *  @param format               формат секции
 *  @param excludeBanners       массив баннеров (MRAdBanner), которые необходимо исключить из показа
 *  @param containerViewController view controller, из которого будет открыто модальное окно с рекламой
 *  @param animated             True, чтобы открывать модальное окно анимаированно
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров в секции с переданным форматом.
 *  @discussion  Если исключить из показа все баннеры секции, то метод вернёт False, а реклама не будет показана.
 */
- (BOOL)startWithFormat:(NSString *)format exludeBanners:(NSArray *)excludeBanners fullscreenInViewContoller:(UIViewController *)containerViewController animated:(BOOL)animated;

/**
 *  Возвращает флаг, указывающий есть ли уведомления в секциях
 *
 *  @return Если в какой-либо секции есть уведомления, то возвращает true, иначе возвращает false
 *  @see MRADSection
 */
- (BOOL)hasNotifications;

/**
 *  Возвращает флаг, указывающий на наличие уведомлений в секции переданного формата
 *
 *  @param format формат секции
 *
 *  @return Если секция переданного формата существует и в ней есть уведомления, то возвращает true, иначе возвращает false
 *  @see MRADSection
 */
- (BOOL)hasNotificationInSectionWithFormat:(NSString *)format;

#pragma mark - Deprecated methods

/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param formats форматы секций
 *  @discussion Загружает данные о рекламе по сети
 *  @deprecated Используйте метод [MRAdManView loadSlot:withFormat:]
 */
- (void)loadSlot:(NSUInteger)slotId withFormats:(NSArray *)formats __attribute__((deprecated));

/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param formats форматы секций
 *  @param ignoreCache если true, то закешированные данные будут проигнорированы
 *  @discussion Загружает данные о рекламе по сети
 *  @deprecated Используйте метод [MRAdManView loadSlot:withFormat:ignoreCache:]
 */
- (void)loadSlot:(NSUInteger)slotId withFormats:(NSArray *)formats ignoreCache:(BOOL)ignoreCache __attribute__((deprecated));

/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param formats форматы секций
 *  @param customParamsProvider  объект с дополнительными параметрами
 *  @discussion Загружает данные о рекламе по сети
 *  @deprecated Используйте метод [MRAdManView loadSlot:withFormat:withCustomParamsProvider:]
 */
- (void)loadSlot:(NSUInteger)slotId withFormats:(NSArray *)formats withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider __attribute__((deprecated));
/**
 *  Загружает данные о рекламе для переданного слота и переданных форматов секций
 *
 *  @param slotId  идентификатор слота
 *  @param formats форматы секций
 *  @param customParamsProvider  объект с дополнительными параметрами
 *  @param ignoreCache если true, то закешированные данные будут проигнорированы
 *  @discussion Загружает данные о рекламе по сети
 *  @deprecated Используйте [MRAdManView loadSlot:withFormat:withCustomParamsProvider:ignoreCache:]
 */
- (void)loadSlot:(NSUInteger)slotId withFormats:(NSArray *)formats withCustomParamsProvider:(MRAdCustomParamsProvider *)customParamsProvider ignoreCache:(BOOL)ignoreCache __attribute__((deprecated));

/**
 *  Очищает закешированные данные для переданного слота и форматов объявлений
 *
 *  @param slotId  Идентификатор слота
 *  @param formats Форматы объявлений
 *  @deprecated Используйте метод [MRAdManView clearCacheForSlot:format:]
 */
- (void)clearCacheForSlot:(NSInteger)slotId formats:(NSArray *)formats __attribute__((deprecated));

@end

#pragma mark - MRAdManViewDelegateCorp

@protocol MRAdManViewDelegateCorp <MRAdManViewDelegate>

@optional
/**
 *  Вызвается, если в результате обработки нажатия баннера приложение должно показать модальный диалог
 *
 *  @param banner         Нажатый баннер
 *  @param viewController UIViewController, который необходимо показать пользователю
 *  @discussion Метод может возвращать либо SKProductViewController, либо UIViewController с UIWebView с загруженной страницей, на которую ведёт баннер. 
 *  Если этот метод реализован, то методы adManViewWillBeginAction:withViewController: и adManViewDidFinishAction: не вызываются.
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

/**
 *  Вызывается при окончании загрузки данных по сети
 *
 *  @param adManView Объект, вызвавший событие
 */
- (void)adManViewDidFinishLoadingSectionsData:(MRAdManView *)adManView;

/**
 *  Вызывается, когда в результате нажатия на баннер в какой-либо секции происходит изменение её статуса уведомлений
 *
 *  @param adManView Объект, вызвавший событие
 *  @param section   Секция, в которой произошло изменение статуса уведомлений
 *  @disucssion При вызове этого метода проверьте свойство [MRADSection hasNotification]. Если значение этого свойства у всех загруженных секци false, то значение hasNotifications [MRAdManView sectionsData] так же изменится на false
 */
- (void)adManView:(MRAdManView *)adManView didUpdateNotificationsInSection:(MRADSection *)section;

@end

@interface MRMyComAdManView : MRAdManView

@end



