//
//  MPAdAlertGestureRecognizer.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

/**
 To trigger this gesture recognizer, the user needs to swipe from left to right, and then right to
 left at least four times in the target view, while keeping the finger in a straight horizontal line.
 See documentation here:
    https://developers.mopub.com/publishers/tools/creative-flagging-tool/#report-a-creative
 */

#import <UIKit/UIKit.h>

@interface MPAdAlertGestureRecognizer : UIGestureRecognizer

/**
 After adding this gesture recognizer to a new view, set minimum tracking distance base on the view size.
 Default is 100.
*/
@property (nonatomic, assign) CGFloat minTrackedDistanceForZigZag;

@end
