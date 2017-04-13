//
//  MPAdAlertGestureRecognizer.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

extern NSInteger const kMPAdAlertGestureMaxAllowedYAxisMovement;

typedef enum
{
    MPAdAlertGestureRecognizerState_ZigRight1,
    MPAdAlertGestureRecognizerState_ZagLeft2,
    MPAdAlertGestureRecognizerState_Recognized
} MPAdAlertGestureRecognizerState;

@interface MPAdAlertGestureRecognizer : UIGestureRecognizer

// default is 4
@property (nonatomic, assign) NSInteger numZigZagsForRecognition;

// default is 100
@property (nonatomic, assign) CGFloat minTrackedDistanceForZigZag;

@property (nonatomic, readonly) MPAdAlertGestureRecognizerState currentAlertGestureState;
@property (nonatomic, readonly) CGPoint inflectionPoint;
@property (nonatomic, readonly) BOOL thresholdReached;
@property (nonatomic, readonly) NSInteger curNumZigZags;

@end
