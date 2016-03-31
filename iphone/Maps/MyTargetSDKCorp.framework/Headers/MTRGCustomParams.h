//
//  MTRGCustomParams.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 22.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum{
    MTRGGenderUnknown,
    MTRGGenderMale,
    MTRGGenderFemale
} MTRGGender;

@interface MTRGCustomParams : NSObject

// Устанавливает возраст пользователя
@property (strong, nonatomic) NSNumber * age;
// Устаналивает пол пользователя
@property (nonatomic) MTRGGender gender;
// Устанавливает язык локализации баннеров
@property (strong, nonatomic) NSString * language;

@end
