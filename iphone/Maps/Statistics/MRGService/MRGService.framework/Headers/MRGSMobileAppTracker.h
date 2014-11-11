//
//  MRGSMobileAppTracker.h
//  MRGServiceFramework
//
//  Created by AKEB on 30.10.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 MobileAppTracker provides the methods to send events and actions to the
 HasOffers servers.
 */
@interface MRGSMobileAppTracker : NSObject

#pragma mark - Track Actions

/** @name Track Actions */

/**
 Record a Track Action for an Event Id or Name.
 @param eventIdOrName The event name or event id.
 @param isId Yes if the event is an Id otherwise No if the event is a name only.
 */
+ (void)trackActionForEventIdOrName:(NSString*)eventIdOrName eventIsId:(BOOL)isId;

@end
