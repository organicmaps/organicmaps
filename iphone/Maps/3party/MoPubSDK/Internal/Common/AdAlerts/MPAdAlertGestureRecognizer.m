//
//  MPAdAlertGestureRecognizer.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAdAlertGestureRecognizer.h"

#import <UIKit/UIGestureRecognizerSubclass.h>

#define kMaxRequiredTrackedDistance 100
#define kDefaultMinTrackedDistance 100
#define kDefaultNumZigZagsForRecognition 4

NSInteger const kMPAdAlertGestureMaxAllowedYAxisMovement = 50;

@interface MPAdAlertGestureRecognizer ()

@property (nonatomic, assign) MPAdAlertGestureRecognizerState currentAlertGestureState;
@property (nonatomic, assign) CGPoint inflectionPoint;
@property (nonatomic, assign) CGPoint startingPoint;
@property (nonatomic, assign) BOOL thresholdReached;
@property (nonatomic, assign) NSInteger curNumZigZags;

@end

@implementation MPAdAlertGestureRecognizer

@synthesize currentAlertGestureState = _currentAlertGestureState;
@synthesize inflectionPoint = _inflectionPoint;
@synthesize thresholdReached = _thresholdReached;
@synthesize curNumZigZags = _curNumZigZags;
@synthesize numZigZagsForRecognition = _numZigZagsForRecognition;
@synthesize minTrackedDistanceForZigZag = _minTrackedDistanceForZigZag;

- (id)init
{
    self = [super init];
    if (self != nil) {
        [self commonInit];
    }

    return self;
}

- (id)initWithTarget:(id)target action:(SEL)action
{
    self = [super initWithTarget:target action:action];
    if (self != nil) {
        [self commonInit];
    }

    return self;
}

- (void)commonInit
{
    self.minTrackedDistanceForZigZag = kDefaultMinTrackedDistance;
    self.numZigZagsForRecognition = kDefaultNumZigZagsForRecognition;
    [self resetToInitialState];
}

- (void)setMinTrackedDistanceForZigZag:(CGFloat)minTrackedDistanceForZigZag
{
    if (_minTrackedDistanceForZigZag != minTrackedDistanceForZigZag) {
        _minTrackedDistanceForZigZag = MIN(minTrackedDistanceForZigZag, kMaxRequiredTrackedDistance);
    }
}

#pragma mark Required Overrides

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
    if ([touches count] != 1) {
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    CGPoint nowPoint = [touches.anyObject locationInView:self.view];
    self.inflectionPoint = nowPoint;
    self.startingPoint = nowPoint;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];

    if (self.state == UIGestureRecognizerStateFailed) {
        return;
    }

    [self updateAlertGestureStateWithTouches:touches];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];

    if ((self.state == UIGestureRecognizerStatePossible) && self.currentAlertGestureState == MPAdAlertGestureRecognizerState_Recognized) {
        self.state = UIGestureRecognizerStateRecognized;
    }

    [self resetToInitialState];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];

    [self resetToInitialState];

    self.state = UIGestureRecognizerStateFailed;
}

- (void)reset
{
    [super reset];

    [self resetToInitialState];
}

- (void)resetToInitialState
{
    self.currentAlertGestureState = MPAdAlertGestureRecognizerState_ZigRight1;
    self.inflectionPoint = CGPointZero;
    self.startingPoint = CGPointZero;
    self.thresholdReached = NO;
    self.curNumZigZags = 0;
}

#pragma mark State Transitions

- (void)handleZigRightGestureStateWithTouches:(NSSet *)touches
{
    CGPoint nowPoint = [touches.anyObject locationInView:self.view];
    CGPoint prevPoint = [touches.anyObject previousLocationInView:self.view];

    // first zig must be to the right, x must increase
    // if the touch has covered enough distance, then we're ready to move on to the next state
    if (nowPoint.x > prevPoint.x && nowPoint.x - self.inflectionPoint.x >= self.minTrackedDistanceForZigZag) {
        self.thresholdReached = YES;
    } else if (nowPoint.x < prevPoint.x) {
        // user has changed touch direction
        if (self.thresholdReached) {
            self.inflectionPoint = nowPoint;
            self.currentAlertGestureState = MPAdAlertGestureRecognizerState_ZagLeft2;
            self.thresholdReached = NO;
        } else {
            // the user changed directions before covering the required distance, fail
            self.state = UIGestureRecognizerStateFailed;
        }
    }
    // else remain in the current state and continue tracking finger movement
}

- (void)handleZagLeftGestureStateWithTouches:(NSSet *)touches
{
    CGPoint nowPoint = [touches.anyObject locationInView:self.view];
    CGPoint prevPoint = [touches.anyObject previousLocationInView:self.view];

    // zag to the left, x must decrease
    // if the touch has covered enough distance, then we're ready to move on to the next state
    if (nowPoint.x < prevPoint.x && self.inflectionPoint.x - nowPoint.x >= self.minTrackedDistanceForZigZag) {
        BOOL prevThresholdState = self.thresholdReached;
        self.thresholdReached = YES;

        // increment once, and only once, after we hit the threshold for the zag
        if (prevThresholdState != self.thresholdReached) {
            self.curNumZigZags++;
        }

        if (self.curNumZigZags >= self.numZigZagsForRecognition) {
            self.currentAlertGestureState = MPAdAlertGestureRecognizerState_Recognized;
        }
    } else if (nowPoint.x > prevPoint.x) {
        // user has changed touch direction
        if (self.thresholdReached) {
            self.inflectionPoint = nowPoint;
            self.currentAlertGestureState = MPAdAlertGestureRecognizerState_ZigRight1;
            self.thresholdReached = NO;
        } else {
            // the user changed directions before covering the required distance, fail
            self.state = UIGestureRecognizerStateFailed;
        }
    }
    // else remain in the current state and continue tracking finger movement
}

- (void)updateAlertGestureStateWithTouches:(NSSet *)touches
{
    // fail gesture recognition if the touch moves outside of our defined bounds
    if (![self touchIsWithinBoundsForTouches:touches] && self.currentAlertGestureState != MPAdAlertGestureRecognizerState_Recognized) {
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    switch (self.currentAlertGestureState) {
        case MPAdAlertGestureRecognizerState_ZigRight1:
            [self handleZigRightGestureStateWithTouches:touches];

            break;
        case MPAdAlertGestureRecognizerState_ZagLeft2:
            [self handleZagLeftGestureStateWithTouches:touches];

            break;
        default:
            break;
    }
}

- (BOOL)validYAxisMovementForTouches:(NSSet *)touches
{
    CGPoint nowPoint = [touches.anyObject locationInView:self.view];

    return fabs(nowPoint.y - self.startingPoint.y) <= kMPAdAlertGestureMaxAllowedYAxisMovement;
}

- (BOOL)touchIsWithinBoundsForTouches:(NSSet *)touches
{
    CGPoint nowPoint = [touches.anyObject locationInView:self.view];

    // 1. use self.view.bounds because locationInView converts to self.view's coordinate system
    // 2. ensure user doesn't stray too far in the Y-axis
    return CGRectContainsPoint(self.view.bounds, nowPoint) && [self validYAxisMovementForTouches:touches];
}

@end
