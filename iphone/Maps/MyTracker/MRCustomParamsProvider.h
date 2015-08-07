//
// Created by Igor Glotov on 22/08/14.
// Copyright (c) 2014 Mail.ru Group. All rights reserved.
// MyTracker, version 1.1.1

#import <Foundation/Foundation.h>
#import "MRAbstractDataProvider.h"

@interface MRCustomParamsProvider : MRAbstractDataProvider

@property (nonatomic, strong) NSString *language;
@property (nonatomic, strong) NSNumber *age;
/**
 * @param gender Пол пользователя, 0 - пол неизвестен, 1 - мужской, 2 - женский
 */
@property (nonatomic, strong) NSNumber *gender;
@property (nonatomic, strong) NSString *mrgsAppId;
@property (nonatomic, strong) NSString *mrgsUserId;
@property (nonatomic, strong) NSString *mrgsDeviceId;
@property (nonatomic, strong) NSArray *icqIds;
@property (nonatomic, strong) NSArray *okIds;
@property (nonatomic, strong) NSArray *vkIds;
@property (nonatomic, strong) NSArray *emails;

- (void)putDataToBuilder:(JSONBuilder *)builder;

@end
