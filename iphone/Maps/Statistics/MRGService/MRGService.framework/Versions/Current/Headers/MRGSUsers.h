//  $Id: MRGSUsers.h 5672 2014-10-21 10:50:20Z a.grachev $
//
//  MRGSUsers.h
//  MRGServiceFramework
//
//  Created by AKEB on 05.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSUsers_
#define MRGServiceFramework_MRGSUsers_

#import <Foundation/Foundation.h>
#import "MRGS.h"

#define userSlotMaxCount 10

enum {
    MRGSUsersErrorNone = 0,
    MRGSUsersErrorSlotCount = 1,
    MRGSUsersErrorUserId = 2,
    MRGSUsersErrorSlot = 3,
    MRGSUsersErrorSave = 4,
};

/**  Класс MRGSUsers. В инициализации не нуждается. Существовать должен только 1 экземпляр класса
 * 
 */
@interface MRGSUsers : NSObject {
@public

@package
    NSMutableString* _currentUserId;
    int _currentSlot;
    int _currentLoginTime;
    int _currentLogoutTime;
    int _currentRegisterTime;
@protected

@private
    NSRecursiveLock* _locker;
}

/** Экземпляр класса MRGSUsers.
 *	@return Возвращает экземпляр класса MRGSUsers
 */
+ (instancetype)sharedInstance;

/** Описание класса.
 *	@return Возвращает описание экземпляра класса в виде NSString
 */
- (NSString*)description;

/** Описание класса.
 *	@return Возвращает описание экземпляра класса в виде NSDictionary
 */
- (NSDictionary*)getDictionary;

#pragma mark -
#pragma mark Регистрация пользователей
/**  @name Регистрация пользователей */

/** Регистрация нового пользователя в свободный слот с генерацией ID пользователя
 * @param error Ошибка при регистрации
 * @return Вернет ID пользователя
 */
- (NSString*)registerNewUser:(NSError**)error;

/** Регистрация нового пользователя в свободный слот с указанным ID пользователя
 * @param error Ошибка при регистрации
 * @param ref Id Пользователя (если приложение уже существует и есть какой-то или какие-то пользователи)
 * @return Вернет ID пользователя
 */
- (NSString*)registerNewUserWithId:(NSString*)ref andError:(NSError**)error;

/** Регистрация нового пользователя в указанный слот с указанным ID пользователя
 * @param error Ошибка при регистрации
 * @param ref Id Пользователя (если приложение уже существует и есть какой-то или какие-то пользователи)
 * @param slot Слот для хранения пользователя
 * @return Вернет ID пользователя
 */
- (NSString*)registerNewUserWithId:(NSString*)ref andSlot:(int)slot andError:(NSError**)error;

/** Регистрация нового пользователя в указанный слот с генерацией ID пользователя
 * @param error Ошибка при регистрации
 * @param slot Слот для хранения пользователя
 * @return Вернет ID пользователя
 */
- (NSString*)registerNewUserInSlot:(int)slot andError:(NSError**)error;

#pragma mark -
#pragma mark Список пользователей
/**  @name Список пользователей */

/** Возвращает список пользователей
 * @return Вернет список пользователей
 */
- (NSArray*)getAllUsers;

/** Возвращает текущего пользователя
 * @return Вернет пользователя под которым был авторизован игрок
 */
- (NSDictionary*)getCurrentUser;

#pragma mark -
#pragma mark Удаление пользователя
/**  @name Удаление пользователя */

/** Удаляем пользователя по его ID
 * @param ref Id Пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)removeUserWIthId:(NSString*)ref;

/** Удаляем пользователя по его слоту
 * @param slot Слот для хранения пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)removeUserWIthSlot:(int)slot;

/** Удаляем всех пользователей
 */
- (void)removeAllUsers;

#pragma mark -
#pragma mark Авторизация пользователя
/**  @name Авторизация пользователя */

/** Авторизация пользователя по его ID
 * @param ref Id Пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)authorizationUserWithId:(NSString*)ref;

/** Выход пользователя
 */
- (void)logoutCurrentUser;

#pragma mark -
#pragma mark Работа с данными пользователя
/**  @name Работа с данными пользователя */

/** Передача пользовательских данных в виде JSON на сервер
 * @param jsonData данные пользователя в виде json строки
 * @return Вернет результат выполнения метода
 */
- (BOOL)sendUserJsonData:(NSString*)jsonData;

/** Сохранение пользовательских данных
 * @param data бинарные данные пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)saveUserData:(NSData*)data;

/** Сохранение пользовательских данных
 * @param object экземпляр одного из классов (NSString, NSNumber, NSDictionary, NSArray, NSNull, NSDate)
 * @return Вернет результат выполнения метода
 */
- (BOOL)saveUserObject:(NSObject*)object;

/** Сохранение пользовательских данных
 * @param data бинарные данные пользователя
 * @param slot Слот для хранения пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)saveUserData:(NSData*)data withSlot:(int)slot;

/** Сохранение пользовательских данных
 * @param object экземпляр одного из классов (NSString, NSNumber, NSDictionary, NSArray, NSNull, NSDate)
 * @param slot Слот для хранения пользователя
 * @return Вернет результат выполнения метода
 */
- (BOOL)saveUserObject:(NSObject*)object withSlot:(int)slot;

/** Загрузка пользовательских данных
 * @return Вернет данные, которые были записаны до этого
 */
- (NSData*)loadUserData;

/** Загрузка пользовательских данных
 * @return Вернет данные, которые были записаны до этого. Экземпляр одного из классов (NSString, NSNumber, NSDictionary, NSArray, NSNull, NSDate)
 */
- (id)loadUserObject;

/** Загрузка пользовательских данных
 * @param slot Слот для хранения пользователя
 * @return Вернет данные, которые были записаны до этого
 */
- (NSData*)loadUserDataWithSlot:(int)slot;

/** Загрузка пользовательских данных
 * @param slot Слот для хранения пользователя
 * @return Вернет данные, которые были записаны до этого. Экземпляр одного из классов (NSString, NSNumber, NSDictionary, NSArray, NSNull, NSDate)
 */
- (id)loadUserObjectWithSlot:(int)slot;

/**
 *   Пометить пользователя как читера
 *
 *   @param failInt Сколько он захотел
 *   @param trueInt Сколько у него на самом деле было
 *   @param comment Комментарий
 */
- (void)markCheaterWithFailInt:(int)failInt andTrue:(int)trueInt andComment:(NSString*)comment;

#pragma mark -
#pragma mark Deprecated methods

/** Экземпляр класса MRGSUsers.
 *	@return Возвращает экземпляр класса MRGSUsers
 *  @deprecated Используйте метод [MRGSUsers sharedInstance]
 */
+ (MRGSUsers*)singleton DEPRECATED_ATTRIBUTE;

/** Отправка пользовательских данных
 * @param userData данные для отправки на сервер
 * @deprecated
 */
- (void)sendUserDataToServer:(NSDictionary*)userData DEPRECATED_ATTRIBUTE;


@end
#endif