//
//  MRTrackerParams.h
//  myTrackerSDK 1.4.9
//
//  Created by Igor Glotov on 20.03.15.
//  Copyright © 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum{
    MRLocationTrackingNone = 0,
    MRLocationTrackingCached,
    MRLocationTrackingActive
} MRLocationTracking;


@interface MRTrackerParams : NSObject

- (instancetype)initWithTrackerId:(NSString *)trackerId;
@property (strong, nonatomic) NSString *trackerId;
@property (nonatomic) BOOL trackAppLaunch;

//Timeout for assuming app was started again - 30..7200 sec.,  default - 30 sec.
@property (nonatomic) NSTimeInterval launchTimeout;

//default = MRLocationTrackingActive
@property (nonatomic) MRLocationTracking locationTracking;

- (void)setLanguage:(NSString *)lang;
- (void)setAge:(NSNumber *)age;
//User gender (0 - unkown, 1 - male, 2 - female)
- (void)setGender:(NSNumber *)gender;

@end
