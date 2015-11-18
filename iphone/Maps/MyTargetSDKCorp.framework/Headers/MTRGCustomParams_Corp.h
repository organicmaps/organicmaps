//
//  MTRGCustomParams_Corp.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Anton Bulankin on 22.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <MyTargetSDKCorp/MTRGCustomParams.h>

@interface MTRGCustomParams ()

#pragma mark -- corp options

// Устанавливает email пользователя
@property (strong, nonatomic) NSString * email;
// Устанавливает номер телефона пользователя
@property (strong, nonatomic) NSString * phone;
//Устанавливает icq идентификатор пользователя
@property (strong, nonatomic) NSString * icqId;
//Устанавливает идентификатор пользователя в Одноклассниках
@property (strong, nonatomic) NSString * okId;
//Устанавливает идентификатор пользователя в VK
@property (strong, nonatomic) NSString * vkId;

#pragma mark - MRGS options (corp)

//MRGS: Устанавливает mrgs-идентификатор приложения
@property (strong, nonatomic) NSString * mrgsAppId;
//MRGS: Устанавливает mrgs-идентификатор пользователя в приложении
@property (strong, nonatomic) NSString * mrgsUserId;
//MRGS: Устанавливает mrgs-идентификатор устройства
@property (strong, nonatomic) NSString * mrgsDeviceId;

-(NSDictionary*) asDictionary;

//Что бы удалить параметр, нужно передать значение nil
-(void) setCustomParam:(NSString*)param forKey:(NSString*)key;
-(NSString*) customParamForKey:(NSString*)key;

@end
