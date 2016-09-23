//
//  MTRGStarsRatingView.h
//  myTargetSDK 4.5.10
//
//  Created by Igor Glotov on 12.08.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface MTRGStarsRatingView : UIView

//rating in interval 0.0....5.0
@property(strong, nonatomic) NSNumber *rating;

- (void)setStarPadding:(CGFloat)starPadding;

@end