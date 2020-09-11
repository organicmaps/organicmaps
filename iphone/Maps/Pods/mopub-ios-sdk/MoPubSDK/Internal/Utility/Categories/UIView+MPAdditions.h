//
//  UIView+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@interface UIView (MPAdditions)

@property (nonatomic) CGFloat mp_x;
@property (nonatomic) CGFloat mp_y;
@property (nonatomic) CGFloat mp_height;
@property (nonatomic) CGFloat mp_width;

- (void)setMp_x:(CGFloat)mp_x;
- (void)setMp_y:(CGFloat)mp_y;
- (void)setMp_width:(CGFloat)mp_width;
- (void)setMp_height:(CGFloat)mp_height;

- (UIView *)mp_snapshotView;

// convert any UIView to UIImage view. We can apply blur effect on UIImage.
- (UIImage *)mp_snapshot:(BOOL)usePresentationLayer;

@end

/**
 This @c MPSafeArea category is for reducing boilerplate code for Safe Area handling.
 */
@interface UIView (MPSafeArea)

@property(nonatomic,readonly) NSLayoutXAxisAnchor *mp_safeLeadingAnchor;
@property(nonatomic,readonly) NSLayoutXAxisAnchor *mp_safeTrailingAnchor;
@property(nonatomic,readonly) NSLayoutXAxisAnchor *mp_safeLeftAnchor;
@property(nonatomic,readonly) NSLayoutXAxisAnchor *mp_safeRightAnchor;
@property(nonatomic,readonly) NSLayoutYAxisAnchor *mp_safeTopAnchor;
@property(nonatomic,readonly) NSLayoutYAxisAnchor *mp_safeBottomAnchor;
@property(nonatomic,readonly) NSLayoutDimension *mp_safeWidthAnchor;
@property(nonatomic,readonly) NSLayoutDimension *mp_safeHeightAnchor;
@property(nonatomic,readonly) NSLayoutXAxisAnchor *mp_safeCenterXAnchor;
@property(nonatomic,readonly) NSLayoutYAxisAnchor *mp_safeCenterYAnchor;

@end
