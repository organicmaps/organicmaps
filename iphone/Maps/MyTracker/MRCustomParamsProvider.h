//
// Created by Igor Glotov on 22/08/14.
// Copyright (c) 2014 Mail.ru Group. All rights reserved.
// MyTracker, version 1.0.18

#import <Foundation/Foundation.h>
#import "MRAbstractDataProvider.h"

@interface MRCustomParamsProvider : MRAbstractDataProvider

- (void)setLanguage:(NSString *)lang;


- (void)setAge:(NSNumber *)age;

/**
* @param gender Пол пользователя, 0 - пол неизвестен, 1 - мужской, 2 - женский
*/
- (void)setGender:(NSNumber *)gender;

- (void)setMRGSAppId:(NSString *)mrgsAppId;

- (void)setMRGSUserId:(NSString *)mrgsUserId;

- (void)setMRGSDeviceId:(NSString *)mrgsDeviceId;

- (void)setPhone:(NSString *)phone;

- (void)setIcqId:(NSString *)icqId;

- (void)setOkId:(NSString *)okId;

- (void)setEmail:(NSString *)email;

@end
