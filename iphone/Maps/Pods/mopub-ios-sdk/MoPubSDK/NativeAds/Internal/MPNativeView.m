//
//  MPNativeView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeView.h"

@implementation MPNativeView

- (void)willMoveToSuperview:(UIView *)superview
{
    [self.delegate nativeViewWillMoveToSuperview:superview];
}

@end
