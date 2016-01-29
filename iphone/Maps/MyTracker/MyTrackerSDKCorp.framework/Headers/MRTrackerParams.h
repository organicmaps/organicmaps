//
//  MRTrackerParams.h
//  myTrackerSDKCorp 1.4.0
//
//  Created by Igor Glotov on 20.03.15.
//  Copyright © 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MRTrackerParams : NSObject

- (instancetype)initWithTrackerId:(NSString *)trackerId;

@property (strong, nonatomic) NSString *trackerId;
//Отслеживать ли Launch
@property (atomic) BOOL trackAppLaunch;
//Timeout for assuming app was started again - 30..7200 sec.,  default - 30 sec.
@property (atomic) NSTimeInterval launchTimeout;


- (void)setLanguage:(NSString *)lang;
- (void)setAge:(NSNumber *)age;

/**
* @param gender Пол пользователя, 0 - пол неизвестен, 1 - мужской, 2 - женский
*/
- (void)setGender:(NSNumber *)gender;

@end
