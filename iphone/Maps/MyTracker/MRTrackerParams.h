//
// Created by Glotov on 20/03/15.
// Copyright (c) 2015 Mail.ru Group. All rights reserved.
// MyTracker, version 1.1.1


#import <Foundation/Foundation.h>

@class MRCustomParamsProvider;


@interface MRTrackerParams : NSObject

- (instancetype)initWithTrackerId:(NSString *)trackerId;

@property (strong, nonatomic) NSString *trackerId;
@property (nonatomic) BOOL trackAppLaunch;

- (void)setLanguage:(NSString *)lang;
- (void)setAge:(NSNumber *)age;

/**
* @param gender Пол пользователя, 0 - пол неизвестен, 1 - мужской, 2 - женский
*/
- (void)setGender:(NSNumber *)gender;

@end
