//
//  MRAdManView.h
//  MRAdMan
//
//  Created by Пучка Илья on 27.01.14.
//  Copyright (c) 2014 MailRu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MRAdManFullScreenViewController.h"
#import "SKStoreProductViewControllerDelegateExtended.h"

@protocol MRAdManViewDelegate;

#pragma mark - MRAdManView

/**
 **MRAdManView** - компонент, реализующий получение данных о рекламе и отображение баннеров, а так же отправку статистики по показам и кликам по баннерам. MRAdManView - сабкласс UIView и содержит в себе subviews, реализущие интерфейс показа баннеров. При загрузке информации кэш не используется (в отличие от MRAdManAdapter, в котором использование кэша при загрузке можно управлять).

*/
@interface MRAdManView : UIView

/**
 *  Делегат MRAdManView
 *  @see MRAdManViewDelegate
 */

@property (nonatomic, weak) id<MRAdManViewDelegate> delegate;

@property (nonatomic) BOOL useNativeShowcase;

/**
 * Если useAppLayout = YES, вписываем банер в заданный размер родительского view
 * Значение по умолчанию = NO, это значит что банер использует свою логику по умолчанию
 */
@property (nonatomic) BOOL useAppLayout;

/**
 *  Загружает объявления из переданного слота с форматом объявлений 320x50
 *
 *  @param slotId Идентификтор слота
 *  @param padId   идентификатор площадки
 */
- (void)loadSlot:(NSUInteger)slotId withPadId:(NSString *)padId;


/**
 *  Загружает объявления из переданного слота с форматом объявлений 320x50
 *
 *  @param slotId Идентификтор слота
 *  @deprecated Используйте [MRAdManView loadSlot:withPadId:]
 */
- (void)loadSlot:(NSUInteger)slotId __attribute__((deprecated));

/**
 *  Загружает объявления из переданного слота с переданным форматом объявлений
 *
 *  @param slotId Идентификатор слота
 *  @param padId   идентификатор площадки
 *  @param format Формат объявлений
 */
- (void)loadSlot:(NSUInteger)slotId withPadId:(NSString *)padId withFormat:(NSString *)format;

/**
 *  Загружает объявления из переданного слота с переданным форматом объявлений
 *
 *  @param slotId Идентификатор слота
 *  @param format Формат объявлений
 *  @deprecated Используйте [MRAdManView loadSlot:withPadId:withFormat:]
 */
- (void)loadSlot:(NSUInteger)slotId withFormat:(NSString *)format __attribute__((deprecated));

/**
 *  Показывает полноэкранный баннер в качестве сплэш-скрина
 *  @param slotId Идентификатор слота
 *  @param padId   идентификатор площадки
 *  @param initialImage Изображение, используемое приложением в качестве сплэш-скрина
 *  @discussion Метод необходимо вызывать из метода application:didFinishLaunchingWithOptions: перед выходом из него
 */
//Метод убран из интерфейса до выяснения надобности SplashScreen
//- (void)displaySplashScreenForSlot:(NSUInteger)slotId withPadId:(NSString *)padId initialImage:(UIImage *)initialImage;

/**
 *  Показывает полноэкранный баннер в качестве сплэш-скрина
 *  @param slotId Идентификатор слота
 *  @param initialImage Изображение, используемое приложением в качестве сплэш-скрина
 *  @discussion Метод необходимо вызывать из метода application:didFinishLaunchingWithOptions: перед выходом из него
 *  @deprecated Используйте [MRAdManView displaySplashScreenForSlot:withPadId:initialImage:]
 */
//Метод убран из интерфейса до выяснения надобности SplashScreen
//- (void)displaySplashScreenForSlot:(NSUInteger)slotId initialImage:(UIImage *)initialImage __attribute__((deprecated));

/**
 *  Возвращает формат загруженных объявлений
 *
 *  @return Формат загруженных объявлений
 */
- (NSString *)format;

/**
*   Начинает показ рекламы
*   @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров, гтовых к показу.
*   @discussion Создает иерархию View->WebView, только для неполноэкранных баннеров, parent view не хранится
*/
- (BOOL)start;

/**
 *  Начинает показ рекламы в переданном view
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров, гтовых к показу.
 */
- (BOOL)startInView:(UIView *)view;

/**
 *  Начинает показы в полноэкранном виде
 *
 *  @return Возвращает true, если показ рекламы начался. Возвращает false, если нет баннеров, гтовых к показу.
 */
- (BOOL) startInFullScreen:(UIViewController*)controller;

/**
 *  Останавливает показ рекламы. Если реклама была развёрнута на весь экран, то MRAdManView автоматически будет свёрнут до исходного размера
 */
- (void)stop;

/**
 *  Приостанавливает показ рекламы (объявление не скрывается с экрана)
 */
- (void)pause;

/**
 *  Возобновляет показ рекламы
 */
- (void)resume;

/**
 *  True, если в данный момент объявление находится в развёрнутом состоянии
 *
 *  @return True, если в данный момент объявление находится в развёрнутом состоянии
 */
- (BOOL)isExpanded;

/**
 *  True, если в данный момент объявление развёрнуто на весь экран
 *
 *  @return True, если в данный момент объявление развёрнуто на весь экран
 */
- (BOOL)isExpandedToFullScreen;

/**
 True, если в данный момент показ рекламы не остановлен
 @return True, если в данный момент показ рекламы не остановлен
 */
- (BOOL)isPaused;

/**
 *  Прерывает текущее действие, произошедшее по нажатию на баннер (сворачивает модальный диалог, если он был показан)
 */
- (void)cancelAction;

/**
 *  Устанавливает режим логирования
 *
 *  @param debugMode True для включения логирования в режиме отладки. По умолчанию логирование отключено
 */
- (void)setDebugMode:(BOOL)debugMode;

/**
 *  Возвращает флаг, указывающий на наличие баннеров в секции переданного формата
 *
 *  @param format формат секции
 *
 *  @return Если секция переданного формата существует и в ней есть баннеры, то возвращает true, иначе возвращает false
 */
- (BOOL)hasAdsWithFormat:(NSString *)format;

/**
 *  Возвращает флуг, указывающий на наличие каких-либо рекламных баннеров в секциях
 *
 *  @return True, если в какой-либо секции есть баннеры, fale, если секций нет или они все пустые
 */
- (BOOL)hasAds;

/**
 *  Возвращает флуг, указывающий на то, что реклама готова к показу
 *
 *  @return True, если инициализация рекламы завершилась и реклама готова к показу.
 */
- (BOOL)isReady;

/**
 *  Если false, то при нажатии на баннер всегда будет вызываться метод делегата adManBannerClicked:withURLToOpen:.
 *  @discussion True нужно передавать, если приложение собирается отображать страницы рекламируемых приложений в самом себе, без перехода в браузер.
 */
@property (nonatomic) BOOL handleLinksInApp;

/**
 *  Таймаут на запрос объявлений
 *  @discussion По умолчания 10 сек.
 */
@property (nonatomic) NSTimeInterval requestTimeout;



#pragma mark - MRAdManView options


@end











#pragma mark - MRAdManViewDelegate

/**
 *  Протокол делегата MRAdManView.
 */
@protocol MRAdManViewDelegate <NSObject>

@required

/**
 *  Вызывается при окончании загрузки данных в UIWebView
 *
 *  @param adManView Объект, вызвавший событие
 *  @param hasAds True, если в загруженных данных есть секции запрошенных форматов с баннерами
 */
- (void)adManViewDidFinishLoad:(MRAdManView *)adManView hasAds:(BOOL)hasAds;

@optional

/**
 *  Вызывается при окончании загрузки данных в UIWebView
 *
 *  @param adManView Объект, вызвавший событие
 *  @deprecated Используйте метод [MRAdManViewDelegate adManViewDidFinishLoad:hasAds:]
 */
- (void)adManViewDidFinishLoad:(MRAdManView *)adManView __attribute__((deprecated));

/**
 *  Вызывается при неудачной попытке загрузки данных
 *
 *  @param adManView Объект, вызвавший событие
 *  @param error     Ошибка, полученная при загрузке данных
 */
- (void)adManView:(MRAdManView *)adManView didFailToLoadWithError:(NSError *)error;

/**
 *  Вызывается, перед тем, как MRAdManView наичнает загрузку данных о рекламе
 *
 *  @param adManView Объект, вызвавший событие
 */
- (void)adManViewWillLoad:(MRAdManView *)adManView;

/**
 *  Вызывается, когда показ рекламных объявлений окончен
 *
 *  @param adManView Объект, вызвавший событие
 */
- (void)adManViewDidStop:(MRAdManView *)adManView;

/**
 *  Вызывается, когда пользователь закрывает баннер
 *
 *  @param adManView Объект, вызвавший событие
 */
- (void)adManViewDidClose:(MRAdManView *)adManView;

/**
 *  Вызывается когда пользователь нажимает на баннер
 *
 *  @param adManView Объект, вызвавший событие
 *  @param willLeave True, если в результате нажатия на баннер произойдёт переход в другое приложение
 *
 *  @return Если метод возвращает False, то дейсствие по нажатию баннера будет отменено, нажатие баннера не будет засчитано
 */
- (BOOL)adManViewActionShouldBegin:(MRAdManView *)adManView willLeaveApplication:(BOOL)willLeave;

/**
 *  Вызывается перед тем, как будет показан модальный диалог с сылкой, на которую ведёт баннер (либо контроллер с браузером, либо SKStoreProductViewController)
 *
 *  @param adManView      Объект, вызвавший событие
 *  @param viewController контроллер, который будет показан
 */
- (void)adManViewWillBeginAction:(MRAdManView *)adManView withViewController:(UIViewController *)viewController;

/**
 *  Вызвается после того, как действие, произошедшее по нажатию на баннер, окончено (закрыт показанный модальный диалог)
 *
 *  @param adManView Объект, вызвавший событие
 */
- (void)adManViewDidFinishAction:(MRAdManView *)adManView;

/**
 *  Вызывается, когда реклама разворачивается
 *
 *  @param adManView Объект, вызвавший событие
 *  @discussion Если fullScreen содержит True,то view с рекламой займёт всё пространсто view, переданного в методе [MRAdManView startWithFormat:inView:], или view контроллера, переданного в методе [MRAdManView startWithFormat:fullScreenInViewContoller:animated:].
 *  При необходимости можно самостоятельно задать размеры MRAdManView.
 */
- (void)adManViewDidExpand:(MRAdManView *)adManView fullScreen:(BOOL)fullScreen;

/**
 *  Вызывается, когда реклама сворачивается
 *
 *  @param adManView Объект, вызвавший событие
 *  При сворачивании view с рекламой примет прежние размеры, которые он имел до разворачивания на весь экран. При необходимости можно самостоятельно задать размеры MRAdManView.
 */
- (void)adManViewDidCollapse:(MRAdManView *)adManView;



@end
