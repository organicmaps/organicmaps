/*
 * CBInPlay.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

#import "Chartboost.h"

/*! @abstract CBInPlay forward declaration. */
@class CBInPlay;

/*!
 @class Chartboost
 
 @abstract
 Provide methods to display and controler Chartboost native advertising types.
 This is a category extension that adds additional functionality to the Chartboost object.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface Chartboost (CBInPlay)

/*!
 @abstract
 Cache a number of InPlay objects for the given CBLocation.

 @param location The location for the Chartboost impression type.

 @discussion This method will first check if there is a locally cached InPlay object set
 for the given CBLocation and, if found, will do nothing. If no locally cached data exists
 the method will attempt to fetch data from the Chartboost API server.
*/
+ (void)cacheInPlay:(CBLocation)location;


/*!
 @abstract
 Determine if a locally cached InPlay object exists for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @return YES if there a locally cached InPlay object, and NO if not.
 
 @discussion A return value of YES here indicates that the corresponding
 getInPlay:(CBLocation)location method will return an InPlay object without making
 additional Chartboost API server requests to fetch data to present.
 */
+ (BOOL)hasInPlay:(CBLocation)location;

/*!
 @abstract
 Return an InPlay object for the given CBLocation.
 
 @param location The location for the Chartboost impression type.
 
 @return CBInPlay object if one exists in the InPlay cache or nil if one is not yet available.
 
 @discussion This method will first check if there is a locally cached InPlay object
 for the given CBLocation and, if found, will return the object using the locally cached data.
 If no locally cached data exists the method will attempt to fetch data from the
 Chartboost API server.  If the Chartboost API server is unavailable
 or there is no eligible InPlay object to present in the given CBLocation this method
 is a no-op.
 */
+ (CBInPlay *)getInPlay:(CBLocation)location;

@end


/*!
 @class CBInPlay
 
 @abstract
 CBInPlay ad type is a native ad type that is left the end user to integrate into their
 applications own custom experiences.  Chartboost acts as a data marshalling system
 and gives the developer access to specific attributes of the ad type.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBInPlay : NSObject

/*! @abstract CBLocation target for the CBInPlay ad. */
@property (nonatomic, readonly) CBLocation location;

/*! @abstract Image byte data for the CBInPlay icon. */
@property (nonatomic, strong, readonly) NSData *appIcon;

/*! @abstract Application name associated with the ad. */
@property (nonatomic, strong, readonly) NSString *appName;

/*!
 @abstract
 Marks the CBInPlay object as shown and notifies the Charboost API servers.
 
 @discussion This method will emit a server request to the Chartboost API servers
 to mark the CBInPlay ad as viewed.  You must send this information to correlate
 with installs driven by the ad.
 */
- (void)show;

/*!
 @abstract
 Marks the CBInPlay object as clicked and notifies the Charboost API servers.
 
 @discussion This method will emit a server request to the Chartboost API servers
 to mark the CBInPlay ad as clicked.  You must send this information to correlate
 with installs driven by the ad.
 */
- (void)click;

@end
