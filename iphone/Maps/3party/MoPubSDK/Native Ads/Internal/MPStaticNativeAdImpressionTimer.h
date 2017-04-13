//
//  MPStaticNativeAdImpressionTimer.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol MPStaticNativeAdImpressionTimerDelegate;

@interface MPStaticNativeAdImpressionTimer : NSObject

@property (nonatomic, weak) id<MPStaticNativeAdImpressionTimerDelegate> delegate;

/*
 * Initializes and returns an object that will tell its delegate when the a certain percentage of the view
 * has been visible for a given amount of time.
 *
 * visibilityPercentage is a float between 0.0 and 1.0. For example, 0.7 represents 70%.
 */

- (instancetype)initWithRequiredSecondsForImpression:(NSTimeInterval)requiredSecondsForImpression requiredViewVisibilityPercentage:(CGFloat)visibilityPercentage;

/*
 * Begins monitoring the view to meet the impression tracking criteria set up in -initWithRequiredSecondsForImpression:requiredViewVisibilityPercentage:.
 *
 * The current visibility duration is not reset when calling this method. If the first view
 * was visible for 1 second, the internal state will maintain that the view its tracking has
 * been visible for 1 second even if this method is called again with a different view.
 */

- (void)startTrackingView:(UIView *)view;

@end

@protocol MPStaticNativeAdImpressionTimerDelegate <NSObject>

@required

/*
 * Called when the required visibility time has been satisfied. This delegate method
 * will only be called once for the lifetime of this object.
 */

- (void)trackImpression;

@end
