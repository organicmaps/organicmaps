//
//  MTRGCustomParams.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 22.12.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString *const kMTRGCustomParamsMediationKey;
extern NSString *const kMTRGCustomParamsMediationAdmob;
extern NSString *const kMTRGCustomParamsMediationMopub;
extern NSString *const kMTRGCustomParamsHtmlSupportKey;

typedef enum
{
	MTRGGenderUnspecified,
	MTRGGenderUnknown,
	MTRGGenderMale,
	MTRGGenderFemale
} MTRGGender;

@interface MTRGCustomParams : NSObject

@property NSNumber *age;
@property(nonatomic) MTRGGender gender;
@property(copy) NSString *language;

@property(copy) NSString *email;
@property(copy) NSString *phone;
@property(copy) NSString *icqId;
@property(copy) NSString *okId;
@property(copy) NSString *vkId;

@property(copy) NSString *mrgsAppId;
@property(copy) NSString *mrgsUserId;
@property(copy) NSString *mrgsDeviceId;

- (NSDictionary *)asDictionary;

- (void)setCustomParam:(NSString *)param forKey:(NSString *)key;

- (NSString *)customParamForKey:(NSString *)key;

@end
