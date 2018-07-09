//
//  MRMyTrackerParams.h
//  myTrackerSDK 1.5.3
//
//  Created by Timur Voloshin on 17.06.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRTrackerParams;
typedef enum
{
	MRGenderUnspecified=-1,
	MRGenderUnknown,
	MRGenderMale,
	MRGenderFemale
} MRGender;

typedef enum
{
	MRLocationTrackingModeNone,
	MRLocationTrackingModeCached,
	MRLocationTrackingModeActive
} MRLocationTrackingMode;

@interface MRMyTrackerParams : NSObject

@property(nonatomic, readonly, copy) NSString *trackerId;
@property(nonatomic) BOOL trackLaunch;
@property(nonatomic) NSTimeInterval launchTimeout;
@property(nonatomic) MRLocationTrackingMode locationTrackingMode;
@property(nonatomic) BOOL trackEnvironment;

@property(nonatomic) MRGender gender;
@property(nonatomic) NSNumber *age;
@property(nonatomic, copy) NSString *language;

@property(nonatomic, copy) NSString *mrgsAppId;
@property(nonatomic, copy) NSString *mrgsUserId;
@property(nonatomic, copy) NSString *mrgsDeviceId;
@property(nonatomic) NSArray<NSString *> *icqIds;
@property(nonatomic) NSArray<NSString *> *okIds;
@property(nonatomic) NSArray<NSString *> *vkIds;
@property(nonatomic) NSArray<NSString *> *emails;
@property(nonatomic) NSArray<NSString *> *customUserIds;

- (instancetype)initWithParams:(MRTrackerParams *)trackerParams;
@end
