//
//  MRExpandModalViewController.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPClosableView.h"
#import "MPForceableOrientationProtocol.h"
@protocol MRExpandModalViewControllerDelegate;

/**
 * `MRExpandModalViewController` is specifically for presenting expanded MRAID ads. Its orientation can be
 * forced via the orientationMask property.
 */
@interface MRExpandModalViewController : UIViewController <MPClosableViewDelegate, MPForceableOrientationProtocol>

/**
 * Initializes the view controller with an orientation mask that defines what orientation
 * the view controller supports. When using an orientation mask on initialization, the view controlller
 * will force the orientation of the device to match the orientation mask if the app supports it.
 */
- (instancetype)initWithOrientationMask:(UIInterfaceOrientationMask)orientationMask;

/**
 * Hides the status bar when called. Every call to hideStatusBar should be matched with a call to
 * restoreStatusBarVisibility. That is, each time hideStatusBar is called, restoreStatusBarVisibility
 * must be called before calling hideStatusBar again. If the methods aren't called in the correct order,
 * consecutive calls to this method become no ops.
 */
- (void)hideStatusBar;

/**
 * This will set the visibility of the status bar based on whether or not the status bar was hidden when hideStatusBar was called.
 * A call to this method should be matched with a call to hideStatusBar.  That is, each call to restoreStatusBarVisibility should
 * be preceded by a call to hideStatusBar. Calling this method consecutively will not affect the status bar beyond the first call.
 */
- (void)restoreStatusBarVisibility;

@end
