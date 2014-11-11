//
//  MRGSSocial.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 31.03.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSSocial_
#define MRGServiceFramework_MRGSSocial_

#import <Foundation/Foundation.h>

/** Идентификатор для Facebook */
extern NSString* const kMRGSSocialFacebook;
/** Идентификатор для Twitter */
extern NSString* const kMRGSSocialTwitter;
/** Идентификатор для Odnoklassniki */
extern NSString* const kMRGSSocialOdnoklassniki;
/** Идентификатор для Мой Мир@Mail.ru */
extern NSString* const kMRGSSocialMailRu;
/** Идентификатор для VKontakte */
extern NSString* const kMRGSSocialVKontakte;
/** Идентификатор для Google+ */
extern NSString* const kMRGSSocialGooglePlus;
/** Идентификатор для Google Play Game Services */
extern NSString* const kMRGSSocialGoogleGames;
/** Идентификатор для Apple GameCenter */
extern NSString* const kMRGSSocialGameCenter;
/** Идентификатор для My.Com */
extern NSString* const kMRGSSocialMyCom;

@class MRGSSocialUserInfo;

@interface MRGSSocial : NSObject
/**
 *  Пользователь залогинился в социальную сеть.
 *  @param social Идентификатор социальной сети
 */
+ (void)trackLoginInSocialNetwork:(NSString*)social;

/**
 *  Пользователь сделал пост (или твит)
 *  @param postId Идентификатор поста (опционально)
 *  @param social Идентификатор социальной сети
 */
+ (void)trackPostWithId:(NSString*)postId inSocialNetwork:(NSString*)social;

/**
 *  Пользователь пригласил друзей в игру
 *  @param count Количество приглашенных друзей (опционально, можно передать 0)
 *  @param social Идентификатор социальной сети
 */
+ (void)trackInviteWithCount:(NSInteger)count inSocialNetwork:(NSString*)social;

/**
 *  Отправка информации о количестве друзей пользователя
 *  @param count Количество друзей в указанной социальной сети
 *  @param params Опционально для Twitter. Можно передать ключ-значение: followers - количество тех, кто читает; followings - количество тех, на кого подписан пользователь;
 *  @param social Идентификатор социальной сети
 */
+ (void)trackFriendsCount:(NSInteger)count withAdditionalParams:(NSDictionary*)params inSocialNetwork:(NSString*)social;

/**
 *  Отправка профиля пользователя.
 *  @param userInfo Описание профиля пользователя
 */
+ (void)trackSocialUserInfo:(MRGSSocialUserInfo*)userInfo;

@end

#endif
