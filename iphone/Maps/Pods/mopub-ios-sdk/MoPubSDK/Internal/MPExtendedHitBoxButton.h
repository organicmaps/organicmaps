//
//  MPExtendedHitBoxButton.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 Extends the hit box of the @c UIButton by the amount of points specified by @c touchAreaInsets
 */
@interface MPExtendedHitBoxButton : UIButton
/**
 The amount of points to extend the hitbox of the button. Positive values indicate that the hitbox is increased beyond
 the bounds of the button.
 */
@property (nonatomic, assign) UIEdgeInsets touchAreaInsets;
@end

NS_ASSUME_NONNULL_END
