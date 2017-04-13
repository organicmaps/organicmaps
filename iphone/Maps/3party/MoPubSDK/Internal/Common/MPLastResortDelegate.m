//
//  MPLastResortDelegate.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
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

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (void)productViewControllerDidFinish:(SKStoreProductViewController *)viewController
{
    [viewController dismissViewControllerAnimated:MP_ANIMATED completion:nil];
}
#endif

@end
