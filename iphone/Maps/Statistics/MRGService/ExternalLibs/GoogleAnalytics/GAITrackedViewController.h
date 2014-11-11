/*!
 @header    GAITrackedViewController.h
 @abstract  Google Analytics for iOS Tracked View Controller Header
 @version   2.0
 @copyright Copyright 2012 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol GAITracker;

/*!
 Extends UIViewController to generate Google Analytics appview calls
 whenever the view appears; this is done by overriding the `viewDidAppear:`
 method. The screen name must be set for any tracking calls to be made.

 By default, this will use [GAI defaultTracker] for tracking calls, but one can
 override this by setting the tracker property.
 */
@interface GAITrackedViewController : UIViewController

/*!
 The tracker on which view tracking calls are be made, or `nil`, in which case
 [GAI defaultTracker] will be used.
 */
@property(nonatomic, assign) id<GAITracker> tracker;
/*!
 The screen name, for purposes of Google Analytics tracking. If this is `nil`,
 no tracking calls will be made.
 */
@property(nonatomic, copy)   NSString *screenName;

@end
