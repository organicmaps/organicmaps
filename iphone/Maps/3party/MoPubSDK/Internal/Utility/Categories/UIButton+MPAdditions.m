//
//  UIButton+MPAdditions.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "UIButton+MPAdditions.h"
#import <objc/runtime.h>

@implementation UIButton (MPAdditions)

- (UIEdgeInsets)mp_TouchAreaInsets
{
    return [objc_getAssociatedObject(self, @selector(mp_TouchAreaInsets)) UIEdgeInsetsValue];
}

- (void)setMp_TouchAreaInsets:(UIEdgeInsets)touchAreaInsets
{
    NSValue *value = [NSValue valueWithUIEdgeInsets:touchAreaInsets];
    objc_setAssociatedObject(self, @selector(mp_TouchAreaInsets), value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
    UIEdgeInsets touchAreaInsets = self.mp_TouchAreaInsets;
    CGRect bounds = self.bounds;
    bounds = CGRectMake(bounds.origin.x - touchAreaInsets.left,
                        bounds.origin.y - touchAreaInsets.top,
                        bounds.size.width + touchAreaInsets.left + touchAreaInsets.right,
                        bounds.size.height + touchAreaInsets.top + touchAreaInsets.bottom);
    return CGRectContainsPoint(bounds, point);
}

@end
