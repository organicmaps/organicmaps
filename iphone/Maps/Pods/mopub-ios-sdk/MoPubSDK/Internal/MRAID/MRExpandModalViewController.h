//
//  MRExpandModalViewController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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

@end
