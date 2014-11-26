//  $Id: MRGServiceInit.h 5816 2014-10-30 09:35:50Z a.grachev $
//  MRGServiceInit.h
//  MRGServiceFramework
//
//  Created by AKEB on 21.09.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import "MRGS.h"
#import <Foundation/Foundation.h>

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_4_0
#import <UIKit/UIKit.h>
#endif

@class MRGSDevice, MRGSApplication, MRGServiceParams;
@protocol MRGSServerDataDelegate;

/** Главный класс библиотеки.
 
 Подключение проекта происходит с этого класса
 
 Пример инициализации
 
	[MRGServiceInit MRGServiceWithAppId:1 
							  andSecret:@"HSdfk9aaGs18vvfdLw&ObukV3#oN1ZvZ"
							  andDelegate:delegate
							  andOptions:[NSDictionary dictionaryWithObjectsAndKeys:
													@"YES",@"debug",
													@"YES",@"locations",
													@"YES",@"pushNotifications",
													@"YES",@"badgeReset",
													@"YES",@"crashReports",
													nil]];
 
 delegate передается классу MRGSServerData, после проверяется наличие новых данных на сервере,
 ответ прийдет в метод делегата:
	-(void) loadServerDataDidFinished:(NSDictionary*)serverData;
 
 для дальнейшей проверки данных на сервере можно использовать 
	[[MRGSServerData singleton] loadData];
*/
@interface MRGServiceInit : NSObject

/** Делегат класса.
 */
@property (nonatomic, weak) id<MRGSServerDataDelegate> delegate;

/** Текущие настройки MRGService. */
@property (readonly, nonatomic, strong) MRGServiceParams *serviceParams;

/** Время на сервере. Если 0 - то не смогли получить!
 *
 */
@property (readonly) NSTimeInterval serverTime;

/** Device Token. Для отправки Пуш нотификаций
 *
 */
@property (readonly, nonatomic, strong) NSString* deviceToken;

/** Экземпляр класса MRGSDevice
 *
 * Содержит информацию о текущем устройстве.
 */
@property (readonly, strong) MRGSDevice* ourDevice;

/** Экземпляр класса MRGSApplication
 *
 * Содержит информацию о текущем приложении.
 */
@property (readonly, strong) MRGSApplication* ourApplication;

/**---------------------------------------------------------------------------------------
 * @name Методы инициализации библиотеки (MRGService)
 *  ---------------------------------------------------------------------------------------
 */

/** Singleton библиотеки
 *
 *	@return Возвращает экземпляр класса MRGServiceInit
 */
+ (instancetype)sharedInstance;

/** Инициализация библиотеки
 * 
 *  Параметры appId и secret нужно взять с сайта https://mrgs.my.com/
 *  Метод для инициализации параметров с использованием MRGService.plist.
 * 
 *	@param appId Id приложения.
 *	@param secret Секретный ключ приложения
 *  @param delegate MRGSServerDataDelegate
 *	@see MRGServiceWithAppId:andSecret:andDelegate:andOptions:
*/
+ (void)MRGServiceWithAppId:(int)appId
                  andSecret:(NSString*)secret
                andDelegate:(id<MRGSServerDataDelegate>)delegate;

/** Инициализация библиотеки
 *
 *  Параметры appId и secret нужно взять с сайта https://mrgs.my.com/
 *
 *
 *	@param mrgsParams Экземпляр класса MRGServiceParams с настройками сервиса.
 *	@param externalParams Настройки сторонних SDK (см. MRGServiceParams.h)
 *  @param delegate Делегат
 */
+ (void)startWithServiceParams:(MRGServiceParams *)mrgsParams
             externalSDKParams:(NSArray *)externalParams
                      delegate:(id <MRGSServerDataDelegate>)delegate;

/** Отправка на сервер MPOP Cookie
 *
 *  @param cookies Массив кук.
 */
+ (void)mmCookieSend:(NSArray*)cookies;

/**
 *   Отправка на север собственных креш репортов
 *
 *   @param description Текст, который нужно отправить на сервер
 */
+ (void)sendHandleException:(NSString*)description;

/**
 *   Отправка на север собственных креш репортов
 *
 *   @param description Текст, который нужно отправить на сервер
 *   @param reason Тег, причина падения
 */
+ (void)sendHandleException:(NSString*)description reason:(NSString*)reason;

#pragma mark - Deprecated properties and methods

/** Параметр отвечающий за вывод в лог
 *
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.debug;
 */
@property (readonly, assign) BOOL debug DEPRECATED_ATTRIBUTE;

/** Параметр отвечающий за обнуление цифры на иконке
 *
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.shouldResetBadge;
 */
@property (readonly, assign) BOOL badgeReset DEPRECATED_ATTRIBUTE;

/** Если параметр установлен в YES, то Фреймворк будет автоматом сам запрашивать доступ на определение геопозиции
 *
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.locationTrackingEnabled;
 */
@property (readonly, assign) BOOL locations DEPRECATED_ATTRIBUTE;

/** Если параметр установлен в YES, то Фреймворк будет автоматически запрашивать доступ к пуш нотификациям
 *
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.allowPushNotificationHooks;
 */
@property (readonly, assign) BOOL pushNotifications DEPRECATED_ATTRIBUTE;

/** Id приложения
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.appId;
 */
@property (readonly, assign) int applicationID DEPRECATED_ATTRIBUTE;

/** Секретный ключ приложения
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance].serviceParams.appSecret;
 */
@property (readonly, nonatomic, strong) NSString* applicationSecret DEPRECATED_ATTRIBUTE;

/** Singleton библиотеки
 *
 *	@return Возвращает экземпляр класса MRGServiceInit, если он был инициализирован до этого
 *  @deprecated Используйте метод [MRGServiceInit sharedInstance];
 */
+ (MRGServiceInit*)singleton DEPRECATED_ATTRIBUTE;

/**
 *  Инициализация библиотеки
 *
 *	@param appId Id приложения.
 *	@param secret Секретный ключ приложения
 *  @param delegate MRGSServerDataDelegate
 *  @param options Параметры инициализации MRGS (см. MRGService.plist - Options).
 *  @param externalSDK Настройки сторонних SDK (см. MRGService.plist - ExternalSDK)
 *  @deprecated Используйте метод [MRGServiceInit startWithServiceParams:externalSDKParams:delegate:];
 */
+ (void)MRGServiceWithAppId:(int)appId
                     secret:(NSString*)secret
                   delegate:(id<MRGSServerDataDelegate>)delegate
                    options:(NSDictionary*)options
                externalSDK:(NSDictionary*)externalSDK DEPRECATED_ATTRIBUTE;

/** Инициализация библиотеки
 *
 *  Параметры appId и secret нужно взять с сайта https://mrgs.my.com/
 *
 *
 *	@param appId Id приложения.
 *	@param secret Секретный ключ приложения
 *  @param delegate MRGSServerDataDelegate
 *  @param options Параметры инициализации MRGS.
 *	@deprecated Используйте метод [MRGServiceInit startWithServiceParams:externalSDKParams:delegate:];
 */
+ (void)MRGServiceWithAppId:(int)appId
                  andSecret:(NSString*)secret
                andDelegate:(id<MRGSServerDataDelegate>)delegate
                 andOptions:(NSDictionary*)options DEPRECATED_ATTRIBUTE;
@end
