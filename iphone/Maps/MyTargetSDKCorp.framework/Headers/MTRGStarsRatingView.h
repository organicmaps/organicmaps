//
//  MTRGStarsRatingView.h
//  myTargetSDKCorp 4.2.5
//
//  Created by Igor Glotov on 12.08.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>



@interface MTRGStarsRatingView : UIView

//Задать рейтинг от 0.0....5.0
@property (strong, nonatomic) NSNumber *rating;
//Расстояние между звездами в пикселях
- (void)setStarPadding:(CGFloat)starPadding;

@end
