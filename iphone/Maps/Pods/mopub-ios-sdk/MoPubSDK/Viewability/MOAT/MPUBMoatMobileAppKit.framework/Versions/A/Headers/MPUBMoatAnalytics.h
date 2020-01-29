//
//  MPUBMoatAnalytics.h
//  MPUBMoatMobileAppKit
//
//  Created by Moat on 6/2/16.
//  Copyright © 2016 Moat. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPUBMoatWebTracker.h"
#import "MPUBMoatNativeDisplayTracker.h"
#import "MPUBMoatVideoTracker.h"

@interface MPUBMoatOptions : NSObject<NSCopying>

@property BOOL locationServicesEnabled;
@property BOOL debugLoggingEnabled;
@property BOOL IDFACollectionEnabled;

@end

@interface MPUBMoatAnalytics : NSObject

+ (instancetype)sharedInstance;

- (void)start;

- (void)startWithOptions:(MPUBMoatOptions *)options;

- (void)prepareNativeDisplayTracking:(NSString *)partnerCode;

@end
