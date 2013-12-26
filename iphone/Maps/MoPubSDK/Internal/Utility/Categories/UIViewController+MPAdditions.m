//
//  UIViewController+MPAdditions.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "UIViewController+MPAdditions.h"

#import "MPGlobal.h"

@implementation UIViewController (MPAdditions)

- (UIViewController *)mp_presentedViewController
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_5_0
    if ([self respondsToSelector:@selector(presentedViewController)]) {
        // For iOS 5 and above.
        return self.presentedViewController;
    }
#endif
    // Prior to iOS 5, the parentViewController property holds the presenting view controller.
    return self.parentViewController;
}

- (UIViewController *)mp_presentingViewController
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_5_0
    if ([self respondsToSelector:@selector(presentingViewController)]) {
        // For iOS 5 and above.
        return self.presentingViewController;
    } else {
        // Prior to iOS 5, the parentViewController property holds the presenting view controller.
        return self.parentViewController;
    }
#endif
    return self.parentViewController;
}

- (void)mp_presentModalViewController:(UIViewController *)modalViewController
                             animated:(BOOL)animated
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_5_0
    if ([self respondsToSelector:@selector(presentViewController:animated:completion:)]) {
        [self presentViewController:modalViewController animated:animated completion:nil];
        return;
    }
#endif

    [self presentModalViewController:modalViewController animated:animated];
}

- (void)mp_dismissModalViewControllerAnimated:(BOOL)animated
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_5_0
    if ([self respondsToSelector:@selector(dismissViewControllerAnimated:completion:)]) {
        [self dismissViewControllerAnimated:animated completion:nil];
        return;
    }
#endif

    [self dismissModalViewControllerAnimated:animated];
}

@end
