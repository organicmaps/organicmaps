//
//  SKStoreProductViewController+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "SKStoreProductViewController+MPAdditions.h"

@implementation SKStoreProductViewController (MPAdditions)

+ (BOOL)canUseStoreProductViewController {
    // @c SKStoreProductViewController cannot be used in an app environment that only
    // supports landscape -- portrait is required, or presenting the view controller
    // will produce an app crash -- so query the usable orientations for the app and
    // report whether @c SKStoreProductViewController is usable.

    // Compute this once and use forever because the application's supported orientations
    // will not change in the app lifetime.

    static BOOL canUseStoreProductViewController = NO;

    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        UIWindow * keyWindow = [UIApplication sharedApplication].keyWindow;
        UIInterfaceOrientationMask appSupportedOrientations = [[UIApplication sharedApplication] supportedInterfaceOrientationsForWindow:keyWindow];

        canUseStoreProductViewController = (appSupportedOrientations & UIInterfaceOrientationMaskPortrait) != 0;
    });

    return canUseStoreProductViewController;
}

@end
