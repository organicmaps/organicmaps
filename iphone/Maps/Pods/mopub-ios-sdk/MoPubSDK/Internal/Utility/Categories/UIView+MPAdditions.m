//
//  UIView+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "UIView+MPAdditions.h"

@implementation UIView (Helper)

- (CGFloat)mp_x
{
    return self.frame.origin.x;
}

- (CGFloat)mp_y
{
    return self.frame.origin.y;
}

- (CGFloat)mp_width
{
    return self.frame.size.width;
}

- (CGFloat)mp_height
{
    return self.frame.size.height;
}

- (void)setMp_x:(CGFloat)mp_x
{
    [self setX:mp_x andY:self.frame.origin.y];
}

- (void)setMp_y:(CGFloat)mp_y
{
    [self setX:self.frame.origin.x andY:mp_y];
}

- (void)setX:(CGFloat)x andY:(CGFloat)y
{
    CGRect f = self.frame;
    self.frame = CGRectMake(x, y, f.size.width, f.size.height);
}


- (void)setMp_width:(CGFloat)mp_width
{
    CGRect frame = self.frame;
    frame.size.width = mp_width;
    self.frame = frame;
}

- (void)setMp_height:(CGFloat)mp_height
{
    CGRect frame = self.frame;
    frame.size.height = mp_height;
    self.frame = frame;
}

- (UIView *)mp_snapshotView
{
    CGRect rect = self.bounds;
    UIGraphicsBeginImageContextWithOptions(rect.size, NO, self.window.screen.scale);
    UIView *snapshotView;
    if ([self respondsToSelector:@selector(snapshotViewAfterScreenUpdates:)]) {
        snapshotView = [self snapshotViewAfterScreenUpdates:NO];
    } else {
        CGContextRef ctx = UIGraphicsGetCurrentContext();
        [self.layer renderInContext:ctx];
        UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
        snapshotView = [[UIImageView alloc] initWithImage:image];
    }
    UIGraphicsEndImageContext();
    return snapshotView;
}

- (UIImage *)mp_snapshot:(BOOL)usePresentationLayer
{
    CGRect rect = self.bounds;
    UIGraphicsBeginImageContextWithOptions(rect.size, NO, self.window.screen.scale);
    if (!usePresentationLayer && [self respondsToSelector:@selector(drawViewHierarchyInRect:afterScreenUpdates:)]) {
        [self drawViewHierarchyInRect:rect afterScreenUpdates:NO];
    } else {
        CGContextRef ctx = UIGraphicsGetCurrentContext();
        if (usePresentationLayer) {
            [self.layer.presentationLayer renderInContext:ctx];
        } else {
            [self.layer renderInContext:ctx];
        }
    }
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image;
}

@end

@implementation UIView (MPSafeArea)

- (NSLayoutXAxisAnchor *)mp_safeLeadingAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.leadingAnchor;
    } else {
        return self.leadingAnchor;
    }
}

- (NSLayoutXAxisAnchor *)mp_safeTrailingAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.trailingAnchor;
    } else {
        return self.trailingAnchor;
    }
}

- (NSLayoutXAxisAnchor *)mp_safeLeftAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.leftAnchor;
    } else {
        return self.leftAnchor;
    }
}

- (NSLayoutXAxisAnchor *)mp_safeRightAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.rightAnchor;
    } else {
        return self.rightAnchor;
    }
}

- (NSLayoutYAxisAnchor *)mp_safeTopAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.topAnchor;
    } else {
        return self.topAnchor;
    }
}

- (NSLayoutYAxisAnchor *)mp_safeBottomAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.bottomAnchor;
    } else {
        return self.bottomAnchor;
    }
}

- (NSLayoutDimension *)mp_safeWidthAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.widthAnchor;
    } else {
        return self.widthAnchor;
    }
}

- (NSLayoutDimension *)mp_safeHeightAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.heightAnchor;
    } else {
        return self.heightAnchor;
    }
}

- (NSLayoutXAxisAnchor *)mp_safeCenterXAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.centerXAnchor;
    } else {
        return self.centerXAnchor;
    }
}

- (NSLayoutYAxisAnchor *)mp_safeCenterYAnchor {
    if (@available(iOS 11, *)) {
        return self.safeAreaLayoutGuide.centerYAnchor;
    } else {
        return self.centerYAnchor;
    }
}

@end
