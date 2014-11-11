//
//  MRGSAppsFlyer.h
//  MRGServiceFramework
//
//  Created by AKEB on 06.11.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 *   AppsFlyer
 */
@interface MRGSAppsFlyer : NSObject

/**
 *   Передать событие в AppsFlyer
 *
 *   @param eventName  Название события
 *   @param eventValue Значение события
 */
+ (void)notifyEvent:(NSString*)eventName eventValue:(NSString*)eventValue;

@end
