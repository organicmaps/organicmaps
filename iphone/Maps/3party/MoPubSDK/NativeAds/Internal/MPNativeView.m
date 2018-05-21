//
//  MPNativeView.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPNativeView.h"

@implementation MPNativeView

- (void)willMoveToSuperview:(UIView *)superview
{
    [self.delegate nativeViewWillMoveToSuperview:superview];
}

@end
