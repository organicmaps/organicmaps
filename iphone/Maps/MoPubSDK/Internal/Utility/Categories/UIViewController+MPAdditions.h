//
//  UIViewController+MPAdditions.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIViewController (MPAdditions)

/*
 * Returns the view controller that is being presented by this view controller.
 */
- (UIViewController *)mp_presentedViewController;

/*
 * Returns the view controller that presented this view controller.
 */
- (UIViewController *)mp_presentingViewController;

/*
 * Presents a view controller.
 */
- (void)mp_presentModalViewController:(UIViewController *)modalViewController
                             animated:(BOOL)animated;

/*
 * Dismisses a view controller.
 */
- (void)mp_dismissModalViewControllerAnimated:(BOOL)animated;

@end
