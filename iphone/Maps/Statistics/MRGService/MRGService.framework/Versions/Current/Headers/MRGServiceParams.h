//
//  MRGServiceParams.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 10.10.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Класс для задания настроек работы MRGService. */
@interface MRGServiceParams : NSObject
/** Идентификатор приложения. */
@property (readonly, nonatomic) NSUInteger appId;
/** Секретный ключ приложения. */
@property (readonly, copy, nonatomic) NSString *appSecret;
/** Включение/отключение отладочных логов MRGService в консоле. */
@property (nonatomic) BOOL debug;
/** Настройка автоматическиого сбороса значка уведомлений при запуске или выхода из бэкграунда. */
@property (nonatomic) BOOL shouldResetBadge;
/** Отметить данный девайс как тестовый. */
@property (nonatomic) BOOL startOnTestDevice;
/** Включение/отключение формирования отчетов о падениях в MRGService. */
@property (nonatomic) BOOL crashReportEnabled;
/** Настройка возможности определения местопожения для MRGService. */
@property (nonatomic) BOOL locationTrackingEnabled;
/** Настйрока возможности обработки полученных push notifications. */
@property (nonatomic) BOOL allowPushNotificationHooks;

/**
 *  Создание объекта настроек для MRGService.
 *
 *  @param appId     Идентификатор приложения.
 *  @param appSecret Секретный ключ приложения.
 *
 *  @return Объект с настройками по умолчанию для MRGSService.
 */
- (instancetype)initWithAppId:(NSUInteger)appId andSecret:(NSString *)appSecret;
@end

/** Базовый класс для настроек сторонних SDK. */
@interface MRGSExternalSDKParams : NSObject
/** Настройка включения/отключения стороннего SDK. */
@property (nonatomic, getter=isEnabled) BOOL enable; // YES by default
@end

/** Класс для настроек Flurry. */
@interface MRGSFlurryParams : MRGSExternalSDKParams
/** Ключ приложения для Flurry. */
@property (readonly, copy, nonatomic) NSString *apiKey;
/** Влючение/отключение отладочных логов Flurry в консоле. */
@property (nonatomic) BOOL debug;
/** Настройка формирования и отправки отчетов о падениях приложения во Flurry. */
@property (nonatomic) BOOL crashReportEnabled;

/**
 *  Создание объекта настроек для Flurry.
 *
 *  @param apiKey Ключ приложения.
 *
 *  @return Объект с настройками Flurry по умолчанию.
 */
- (instancetype)initWithAPIKey:(NSString *)apiKey;
@end

/** Класс для настроек MyTracker. */
@interface MRGSMyTrackerParams : MRGSExternalSDKParams
/** Идентификатор приложения для MyTracker. */
@property (readonly, copy, nonatomic) NSString *appId;
/** Включение/отключение отладочных логов MyTracker в консоле. */
@property (nonatomic) BOOL enableLogging;

/**
 *  Создание объекта настроек для MyTracker.
 *
 *  @param appId Идентификатор приложения.
 *
 *  @return Объект с настройками MyTracker по умолчанию.
 */
- (instancetype)initWithAppId:(NSString *)appId;
@end

/** Класс для настроек Adman. */
@interface MRGSAdmanParams : MRGSExternalSDKParams
/** Идентификатор слота приложения в Adman. */
@property (readonly, nonatomic) NSUInteger slotId;
/** Включение/отключение отладочных логов Adman в консоле. */
@property (nonatomic) BOOL debug;

/**
 *  Создание объекта настроек для Adman.
 *
 *  @param slotId Идентификатор слота приложения.
 *
 *  @return Объект с настройками Adman по умолчанию.
 */
- (instancetype)initWithSlotId:(NSUInteger)slotId;
@end

/** Класс для настроек Chartboost. */
@interface MRGSChartboostParams : MRGSExternalSDKParams
/** Идентификатор приложения для Chartboost. */
@property (readonly, copy, nonatomic) NSString *appId;
/** Подпись приложения для Chartboost. */
@property (readonly, copy, nonatomic) NSString *appSignature;

/**
 *  Создание объекта настроек для Chartboost.
 *
 *  @param appId        Идентификатор приложения.
 *  @param appSignature Подпись приложения.
 *
 *  @return Объект с настройками Chartboost по умолчанию.
 */
- (instancetype)initWithAppId:(NSString *)appId andSignature:(NSString *)appSignature;
@end

/** Класс для настроек AppsFlyer. */
@interface MRGSAppsFlyerParams : MRGSExternalSDKParams
/** Идентификационный ключ разработчика для AppsFlyer. */
@property (readonly, copy, nonatomic) NSString *appsFlyerDevKey;
/** Идентификатор приложения для AppsFlyer. */
@property (readonly, copy, nonatomic) NSString *appleAppId;
/** Включение/отключение отладочных логов AppsFlyer в консоле. */
@property (nonatomic) BOOL debug;

/**
 *  Создание объекта настроек для AppsFlyer.
 *
 *  @param devKey Идентификационный ключ разработчика.
 *  @param appId  Идентификатор приложения.
 *
 *  @return Объект с настройками AppsFlyer по умолчанию.
 */
- (instancetype)initWithDevKey:(NSString *)devKey andAppleAppId:(NSString *)appId;
@end

/** Класс для настроек Google Analytics. */
@interface MRGSGoogleAnalyticsParams : MRGSExternalSDKParams
/** Идентификатор приложения для Google Analytics. */
@property (readonly, copy, nonatomic) NSString *trackingId;
/** Включение/отключение обработкиexсpetion-ов в GoogleAnalytics. */
@property (nonatomic) BOOL exceptionHandlerEnabled;
/** Уровень логирования Google Analytics (от 0(минимум логов в консоле) до 4(наиболее подробные логи)). */
@property (nonatomic) NSUInteger logLevel;

/**
 *  Создание объекта настроек для Google Analytics.
 *
 *  @param trackingId Идентификатор приложения.
 *
 *  @return Объект с настройками Google Analytics по умолчанию.
 */
- (instancetype)initWithTrackingId:(NSString *)trackingId;
@end

/** Класс для настроек Google AdWords Conversion Tracking. */
@interface MRGSGoogleConversionTrackingParams : MRGSExternalSDKParams
/** Идентификатор приложения для Google Conversion Tracking. */
@property (readonly, copy, nonatomic) NSString *conversionId;
/** Идентификатор для отслеживания первого запуска. */
@property (copy, nonatomic) NSString *firstRunLabel;
/** Идентификатор для отслеживания эффективной регистрации. */
@property (copy, nonatomic) NSString *effectiveRegistrationLabel;
/** Идентификатор для отслеживания первой покупки в приложении. */
@property (copy, nonatomic) NSString *firstPurchaseLabel;

/**
 *  Создание объекта настроек для Google Conversion Tracking.
 *
 *  @param conversionId Идентификатор приложения.
 *
 *  @return Объект с настройками Google Conversion Tracking по умолчанию.
 */
- (instancetype)initWithConversionId:(NSString *)conversionId;
@end

/** Класс для настроек MobileAppTracking */
@interface MRGSMobileAppTrackingParams : MRGSExternalSDKParams
/** Идентификатор рекламы для MAT. */
@property (readonly, copy, nonatomic) NSString *advertiserId;
/** Идентификатор конверсии для MAT. */
@property (readonly, copy, nonatomic) NSString *conversionKey;
/** Включение/отключение отладочных логов MAT в консоле. */
@property (nonatomic) BOOL debug;
/** Включение/отключение возможности дублирования запросов. Только для отладочных целей. */
@property (nonatomic) BOOL allowDuplicateRequests;

/**
 *  Создание объекта настроек для MobileAppTracking.
 *
 *  @param advertiserId  Идентификатор рекламы.
 *  @param conversionKey Идентификатор конверсии.
 *
 *  @return Объект с настройками MobileAppTracking по умолчанию.
 */
- (instancetype)initWithAdvertiserId:(NSString *)advertiserId
                    andConversionKey:(NSString *)conversionKey;
@end

