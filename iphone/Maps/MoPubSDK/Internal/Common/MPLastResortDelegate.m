//
//  MPLastResortDelegate.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPLastResortDelegate.h"
#import "MPGlobal.h"
#import "UIViewController+MPAdditions.h"

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

- (void)eventEditViewController:(EKEventEditViewController *)controller didCompleteWithAction:(EKEventEditViewAction)action
{
    [controller mp_dismissModalViewControllerAnimated:MP_ANIMATED];
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(NSInteger)result error:(NSError*)error
{
    [controller mp_dismissModalViewControllerAnimated:MP_ANIMATED];
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (void)productViewControllerDidFinish:(SKStoreProductViewController *)viewController
{
    [viewController mp_dismissModalViewControllerAnimated:MP_ANIMATED];
}
#endif

@end
