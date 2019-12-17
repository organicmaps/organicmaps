//
//  FlurryCCPA.h
//  Flurry
//
//  Created by Hunter Hays on 9/5/19.
//  Copyright © 2019 Oath Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface FlurryCCPA : NSObject

/*!
*@brief An api to send ccpa compliance data to Flurry on the user's choice to opt out or opt in to data sale to third parties.
*   @since 10.1.0
*
*
* @param isOptOut   boolean true if the user wants to opt out of data sale, the default value is false
*/
+(void) setDataSaleOptOut: (BOOL) isOptOut;

/*!
*@brief An api to allow the user to request Flurry delete their collected data from this app.
*   @since 10.1.0
*
*/
+(void) setDelete;

@end
