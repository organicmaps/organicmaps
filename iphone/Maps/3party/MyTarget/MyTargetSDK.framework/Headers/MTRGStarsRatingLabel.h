//
//  MTRGStarsRatingLabel.h
//  MyTargetSDK
//
//  Created by Andrey Seredkin on 27.01.17.
//  Copyright © 2017 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MTRGStarsRatingLabel : UILabel

- (instancetype)initWithRating:(NSNumber *)rating; //rating in interval 0...5

@end
