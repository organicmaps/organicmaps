//
//  MTRGImageData.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGMediaData.h>

@interface MTRGImageData : MTRGMediaData

@property(nonatomic, readonly) UIImage *image;

- (instancetype)initWithImage:(UIImage *)image;
@end
