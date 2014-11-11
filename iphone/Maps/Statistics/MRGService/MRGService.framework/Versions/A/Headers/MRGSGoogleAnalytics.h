//
//  MRGSGoogleAnalytics.h
//  MRGServiceFramework
//
//  Created by AKEB on 31.10.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/*!
 Google Analytics iOS top-level class. Provides facilities to create trackers
 and set behaviorial flags.
 */
@interface MRGSGoogleAnalytics : NSObject

/**
 *   Передать в GoogleAnalytics новый экран
 *
 *   @param screenName Название экрана
 */
+ (void)setNewScreenName:(NSString*)screenName;

/**
 *   Event Tracking
 *
 *   @param category The event category
 *   @param action   The event action
 *   @param label    The event label
 *   @param value    The event value
 */
+ (void)createEventWithCategory:(NSString*)category action:(NSString*)action label:(NSString*)label value:(NSNumber*)value;

/**
 *   Social Interactions
 *
 *   @param network The social network with which the user is interacting (e.g. Facebook, Google+, Twitter, etc.).
 *   @param action  The social action taken (e.g. Like, Share, +1, etc.).
 *   @param target  The content on which the social action is being taken (i.e. a specific article or video).
 */
+ (void)createSocialWithNetwork:(NSString*)network action:(NSString*)action target:(NSString*)target;

/**
 *   User Timings
 *
 *   @param category       The category of the timed event
 *   @param intervalMillis  The timing measurement in milliseconds
 *   @param name           The name of the timed event
 *   @param label          The label of the timed event
 */
+ (void)createTimingWithCategory:(NSString*)category interval:(NSNumber*)intervalMillis name:(NSString*)name label:(NSString*)label;

@end
