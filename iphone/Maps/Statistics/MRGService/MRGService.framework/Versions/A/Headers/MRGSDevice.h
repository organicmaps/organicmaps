//	$Id: MRGSDevice.h 5656 2014-10-20 10:48:36Z a.grachev $
//  MRGSDevice.h
//  MRGServiceFramework
//
//  Created by AKEB on 24.09.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

extern NSString* const MRGSReceivedIDFANotification;

/** Информация о системе.
 * В этом классе собрана вся необходимая информация о системе
 *
 */
@interface MRGSDevice : NSObject<CLLocationManagerDelegate>

#pragma mark Размеры

/** Ширина экрана устройства (толкьо в портретной ориентации) */
@property (readonly) CGFloat screenWidth;

/** Высота экрана устройства (толкьо в портретной ориентации) */
@property (readonly) CGFloat screenHeight;

/** Доступная ширина для приложения (толкьо в портретной ориентации) */
@property (readonly) CGFloat applicationWidth;

/** Доступная выстора для приложения (толкьо в портретной ориентации) */
@property (readonly) CGFloat applicationHeight;

/** Масштаб экрана. Если 2 - то Retina Дисплей
 *
 * На MacBookPro10,1 при разрешении 1920x1200 Все равно выдавал число 2. Это очень странно!!!.
 */
@property (readonly) CGFloat screenScale;

#pragma mark Устройство
/**  @name Устройство */

/** Имя устройста. Так, как задал пользователь. Например "My iPhone" */
@property (nonatomic, readonly, copy) NSString* name;

/** Модель устройста. Например "iPhone", "iPod touch" */
@property (nonatomic, readonly, copy) NSString* model;

/** Локализованная Модель устройста. Например "iPhone", "iPod touch" */
@property (nonatomic, readonly, copy) NSString* localizedModel;

/** Название операционной системы Например "iPhone OS" */
@property (nonatomic, readonly, copy) NSString* systemName;

/** Платформа системы */
@property (nonatomic, readonly, copy) NSString* platform;

/** Если параметр установлен в YES, то устройство будет помечено как тестовое
 *
 */
@property (nonatomic) BOOL testDevice;

/** a UUID that may be used to uniquely identify the device, same across apps from a single vendor.
 *
 * - ID that is identical between apps from the same developer.
 * - Erased with removal of the last app for that Team ID.
 * - Backed up.
 *
 */
@property (nonatomic, readonly, copy) NSString* identifierForVendor;

/** An alphanumeric string unique to each device, used only for serving advertisements.
 *
 * Unlike the identifierForVendor property of the MRGSDevice, the same value is returned to all vendors. This identifier may change—for example, if the user erases the device—so you should not cache it.
 *
 */
@property (nonatomic, readonly, copy) NSString* advertisingIdentifier;

/** Уникальный UDID устройства (Можно использовать для идентификации устройства) */
@property (nonatomic, readonly, copy) NSString* odin1;

/** Уникальный UDID устройства */
@property (nonatomic, readonly, copy) NSString* udid;

#pragma mark Сеть
/**  @name Сеть */

/** MD5 от Mac Адреса устройства */
@property (nonatomic, readonly, copy) NSString* macAddress;

/** Mac Адрес устройства */
@property (nonatomic, readonly, copy) NSString* macAddressWiFi;

/** тип подключения к интернету */
@property (nonatomic) int reachability;

/**оператор мобильной связи*/
@property (nonatomic, retain) NSString* carrier;

/** Словарь, в котором находиться информация о мак-адресе устройства и о айпи адресе
 *
 *  ПРИМЕР
 *		netInfo = {
 *			en0 = {
 *				adapter = en0;
 *				ip = "192.168.0.1";
 *				mac = "24:C9:D8:88:2C:AB";
 *			};
 *		};
 *
 */
//@property(nonatomic,readonly,retain) NSMutableDictionary *netInfo;

#pragma mark Локация
/**  @name Локация */

/** Словарь, в котором находиться информация о локации устройства
 *
 */
@property (nonatomic, readonly, strong) NSDictionary* locationInfo;

/** Экземпляр класса MRGSDevice. Содержит информацию о текущем устройстве
 *
 *
 *	@return Возвращает экземпляр класса MRGSDevice
 */
+ (MRGSDevice*)currentDevice;

/** YES если запущено под iPhone */
+ (BOOL)is_iPhone;

/** YES если запущено под iPad */
+ (BOOL)is_iPad;

/** YES если запущено на симуляторе */
+ (BOOL)is_Simulator;

/** YES если запущено под MacOS */
+ (BOOL)is_MacOS;

/** Текущее время на устройстве. Формат unixtimestamp */
+ (int)currentTime;

/** @return Часовой пояс на устройстве */
+ (NSString*)timeZone;

/** @return Язык выбранный на устройстве */
+ (NSString*)language;

/** @return Страна выбранная на устройстве */
+ (NSString*)country;

/** @return Уникальный UDID устройства (Можно использовать для идентификации устройства) */
+ (NSString*)openUDID;

/** Сколько памяти использовано приложением */
+ (NSString*)HwMemoryUse;

/** Сколько всего памяти на устройстве */
+ (NSString*)HwMemoryMax;

/** Версия операционной системы Например "6.0" */
+ (NSString*)systemVersion;

/** Описание класса.
 *
 *	@return Возвращает описание экземпляра класса в виде NSString
 */
- (NSString*)description;

/** Описание класса.
 *
 *	@return Возвращает описание экземпляра класса в виде NSDictionary
 */
- (NSDictionary*)getDictionary;

/** @return Уникальный UDID устройства (Можно использовать для идентификации устройства) */
- (NSString*)openUDID;

/**
 *   identifierForVendor
 *
 *   @return [[UIDevice currentDevice] identifierForVendor]
 */
- (NSString*)idfa;

/** Обновление данных системы */
- (void)update;

/** Стоит на устройстве JailBreak или нет
 *
 *	@return Возвращает статус YES - Jailbreak найден
 */
- (BOOL)isDeviceJailbreak;

@end
