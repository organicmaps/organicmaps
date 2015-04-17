//
//  MWMSpringAnimation.h
//  Maps
//
//  Created by v.mikhaylenko on 21.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MWMAnimator.h"

@interface MWMSpringAnimation : NSObject <Animation>

@property (nonatomic, readonly) CGPoint velocity;

+ (instancetype)animationWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity;

@end
