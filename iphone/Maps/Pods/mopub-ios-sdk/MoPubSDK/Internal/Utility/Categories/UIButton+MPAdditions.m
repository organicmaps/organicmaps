//
//  UIButton+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "UIButton+MPAdditions.h"

@implementation UIButton (MPVideoPlayer)

- (void)applyMPVideoPlayerBorderedStyleWithTitle:(NSString *)title {
    self.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.3];
    self.contentEdgeInsets = UIEdgeInsetsMake(8, 32, 8, 32);
    self.titleLabel.font = [UIFont systemFontOfSize:16];
    self.layer.borderColor = UIColor.grayColor.CGColor;
    self.layer.borderWidth = 1;
    self.layer.cornerRadius = 4;
    [self setTitle:title forState:UIControlStateNormal];
    [self sizeToFit];
}

@end
