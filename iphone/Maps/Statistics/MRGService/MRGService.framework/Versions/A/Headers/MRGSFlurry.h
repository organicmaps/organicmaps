//
//  MRGSFlurry.h
//  MRGServiceFramework
//
//  Created by AKEB on 25.11.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 *   Flurry
 */
@interface MRGSFlurry : NSObject

/**
 *  This method overrides #logEvent to allow you to associate parameters with an event. Parameters
 *  are extremely valuable as they allow you to store characteristics of an action. For example,
 *  if a user purchased an item it may be helpful to know what level that user was on.
 *  By setting this parameter you will be able to view a distribution of levels for the purcahsed
 *  event on the <a href="http://dev.flurry.com">Flurrly Dev Portal</a>.
 *
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters A map containing Name-Value pairs of parameters.
 */
+ (void)logEvent:(NSString*)eventName withParameters:(NSDictionary*)parameters;

/**
 *  This method overrides #logEvent to allow you to capture the length of an event with parameters.
 *  This can be extremely valuable to understand the level of engagement with a particular action
 *  and the characteristics associated with that action. For example, you can capture how long a user
 *  spends on a level or reading an article. Parameters can be used to capture, for example, the
 *  author of an article or if something was purchased while on the level.
 *
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters A map containing Name-Value pairs of parameters.
 *  @param timed Specifies the event will be timed.
 */
+ (void)logEvent:(NSString*)eventName withParameters:(NSDictionary*)parameters timed:(BOOL)timed;

/**
 *  This method ends an existing timed event.  If parameters are provided, this will overwrite existing
 *  parameters with the same name or create new parameters if the name does not exist in the parameter
 *  map set by #logEvent:withParameters:timed:.
 *
 *  @param eventName Name of the event. For maximum effectiveness, we recommend using a naming scheme
 *  that can be easily understood by non-technical people in your business domain.
 *  @param parameters A map containing Name-Value pairs of parameters.
 */
+ (void)endTimedEvent:(NSString*)eventName withParameters:(NSDictionary*)parameters; // non-nil parameters will update the parameters

@end
