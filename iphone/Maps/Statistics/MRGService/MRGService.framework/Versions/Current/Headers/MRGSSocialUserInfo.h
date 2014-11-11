//
//  MRGSSocialUserInfo.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 31.03.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSSocialUserInfo_
#define MRGServiceFramework_MRGSSocialUserInfo_

#import <Foundation/Foundation.h>

/** Класс для описания профиля пользователя в социальной сети. */
@interface MRGSSocialUserInfo : NSObject

/** Идентификатор социальной сети. */
@property (nonatomic, copy) NSString* socialNetwork;
/** Идентификатор пользователя в социальной сети. */
@property (nonatomic, copy) NSString* userId;
/** Имя пользователя. */
@property (nonatomic, copy) NSString* name;
/** Ник пользователя. */
@property (nonatomic, copy) NSString* nick;
/** Ссылка на аватрку пользователя. */
@property (nonatomic, copy) NSString* avatarUrl;
/** День рождeния пользователя. */
@property (nonatomic) NSInteger birthDateDay;
/** Месяц рождeния пользователя. */
@property (nonatomic) NSInteger birthDateMonth;
/** Год рождения пользователя. */
@property (nonatomic) NSInteger birthDateYear;
/** Пол пользователя. */
@property (nonatomic, copy) NSString* gender;
/** Токен доступа в социальной сети. */
@property (nonatomic, copy) NSString* accessToken;
/** Секретный ключ доступа в социальной сети. */
@property (nonatomic, copy) NSString* tokenSecret;
/** Дата окончания действия токена (в unixtime). */
@property (nonatomic) NSTimeInterval expirationDate;

@end

#endif