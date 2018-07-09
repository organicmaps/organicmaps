//
//  UIView+MPAdditions.h
//  Copyright (c) 2015 MoPub. All rights reserved.
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
