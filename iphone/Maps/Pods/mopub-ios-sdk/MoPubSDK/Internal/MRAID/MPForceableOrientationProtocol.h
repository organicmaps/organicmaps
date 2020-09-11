//
//  MPForceableOrientationProtocol.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@protocol MPForceableOrientationProtocol <NSObject>

/**
 * An orientation mask that defines the orientations the view controller supports.
 * This cannot force a change in orientation though.
 */
- (void)setSupportedOrientationMask:(UIInterfaceOrientationMask)supportedOrientationMask;

@end
