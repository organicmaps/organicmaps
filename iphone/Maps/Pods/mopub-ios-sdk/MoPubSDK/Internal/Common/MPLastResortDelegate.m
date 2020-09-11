//
//  MPLastResortDelegate.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPLastResortDelegate.h"
#import "MPGlobal.h"

@class MFMailComposeViewController;

@implementation MPLastResortDelegate

+ (id)sharedDelegate
{
    static MPLastResortDelegate *lastResortDelegate;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        lastResortDelegate = [[self alloc] init];
    });
    return lastResortDelegate;
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(NSInteger)result error:(NSError*)error
{
    [(UIViewController *)controller dismissViewControllerAnimated:MP_ANIMATED completion:nil];
}

- (void)productViewControllerDidFinish:(SKStoreProductViewController *)viewController
{
    [viewController dismissViewControllerAnimated:MP_ANIMATED completion:nil];
}

@end
