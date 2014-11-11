/*
 * CBMoreAppsBadge.h
 * Chartboost
 * 5.0.3
 *
 * Copyright 2011 Chartboost. All rights reserved.
 */

/*!
 @class CBMoreAppsBadge
 
 @abstract
 Class for creating a UIVIew that will display the number of applications displayed
 on the more applications page. It is meant to be placed over a button that will display the more applications page.
 
 @discussion For more information on integrating and using the Chartboost SDK
 please visit our help site documentation at https://help.chartboost.com
 */
@interface CBMoreAppsBadge : UIView

/*!
 @abstract
 Returns a badge view with the number of unviewed applications on the More Applications page.
 
 @return CBMoreAppsBadge a badge view.
 
 @discussion This method returns a customizable badge that by default displays the number of applications displayed
 on the more applications page. It is meant to be placed over a button that will display the more applications page.
 */
+ (CBMoreAppsBadge *)moreAppsBadge;

@end
