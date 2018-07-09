//
//  MPForceableOrientationProtocol.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol MPForceableOrientationProtocol <NSObject>

/**
 * An orientation mask that defines the orientations the view controller supports.
 * This cannot force a change in orientation though.
 */
- (void)setSupportedOrientationMask:(UIInterfaceOrientationMask)supportedOrientationMask;

@end
