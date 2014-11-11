//
//  MRAdCustomParamsProvider.h
//  MRAdMan
//
//  Created by Пучка Илья on 20.01.14.
//  Copyright (c) 2014 MailRu. All rights reserved.
//

/*
 Класс для передачи дополнительных параметров в запросах рекламы и трэкинга установок
 */
@interface MRAdCustomParamsProvider : NSObject <NSCopying>

/*
 Устанавливает язык локализации баннеров
 */
- (void)setLanguage:(NSString *)lang;

#pragma mark - User info
/* 
 Устанавливает email пользователя
 */
- (void)setEmail:(id)email;

/* 
 Устанавливает номер телефона пользователя
 */
- (void)setPhone:(id)phone;

/* 
 Устанавливает icq идентификатор пользователя
 */
- (void)setIcqId:(id)icqId;

/* 
 Устанавливает идентификатор пользователя в Одноклассниках
 */
- (void)setOkId:(id)okId;

/*
 Устанавливает возраст пользователя
 */
- (void)setAge:(NSNumber *)age;

/**
* Устаналивает пол пользователя
* @param gender Пол пользователя, 0 - пол неизвестен, 1 - мужской, 2 - женский
*/
- (void)setGender:(NSNumber *)gender;

#pragma mark - MRGS info
/*
 Устанавливает mrgs айди приложения
 */
- (void)setMRGSAppId:(id)mrgsAppId;

/*
 Устанавливает mrgs айди пользователя в приложении
 */
- (void)setMRGSUserId:(id)mrgsUserId;

/* 
 Устанавливает mrgs айди устройства
 */
- (void)setMRGSDeviceId:(id)mrgsDeviceId;

@end
