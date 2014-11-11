//  $Id: MRGSApplication.h 5656 2014-10-20 10:48:36Z a.grachev $
//  MRGSApplication.h
//  MRGServiceFramework
//
//  Created by AKEB on 28.09.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

/** В этом классе собрана вся информация о приложении. */
@interface MRGSApplication : NSObject
/**  Идентификатор приложения (Bundle ID) */
@property (nonatomic, copy, readonly) NSString* applicationBundleIdentifier;
/** Название приложения */
@property (nonatomic, copy, readonly) NSString* applicationBundleName;
/** Название приложения (отображаемое под иконкой) */
@property (nonatomic, copy, readonly) NSString* applicationBundleDisplayName;
/** Версия приложения */
@property (nonatomic, copy, readonly) NSString* applicationVersion;
/** Версия билда */
@property (nonatomic, copy, readonly) NSString* applicationBuild;
/** Кол-во секунд, которое пользователь потратил в приложении за все время */
@property (readonly) int allSessions;
/** Кол-во секунд, которое пользователь потратил в приложении за сегодня */
@property (readonly) int todaySession;

/** Экземпляр класса MRGSApplication. Содержит информацию о текущем приложении
 *
 *
 *	@return Возвращает экземпляр класса MRGSApplication
 */
+ (MRGSApplication*)currentApplication;

/** Описание класса.
 *
 *	@return Возвращает описание экземпляра класса в виде NSDictionary
 */
- (NSDictionary*)getDictionary;

/** Обновление параметров */
- (void)update;

/**
 *  Отметить приожение как обновленное.
 *
 *  @param date Дата регистрации (первой установки) приложения. Необязательный параметр.
 */
- (void)markAsUpdatedWithRegistrationDate:(NSDate*)date;

@end
