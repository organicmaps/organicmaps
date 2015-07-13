//
//  UIButton+RuntimeAttributes.h
//  Maps
//
//  Created by Ilya Grechuhin on 09.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIButton (RuntimeAttributes)

@property (nonatomic) NSString * localizedText;

- (void)setBackgroundColor:(UIColor *)color forState:(UIControlState)state;

@end