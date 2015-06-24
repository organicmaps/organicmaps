//
//  MWMPlacePage+Animation.h
//  Maps
//
//  Created by v.mikhaylenko on 19.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePage.h"
#import "MWMSpringAnimation.h"

@class MWMSpringAnimation;

@interface MWMPlacePage (Animation)

@property (nonatomic) MWMSpringAnimation * springAnimation;

- (void)cancelSpringAnimation;
- (void)startAnimatingPlacePage:(MWMPlacePage *)placePage initialVelocity:(CGPoint)velocity completion:(MWMSpringAnimationCompletionBlock)completion;
- (CGPoint)targetPoint;

@end
