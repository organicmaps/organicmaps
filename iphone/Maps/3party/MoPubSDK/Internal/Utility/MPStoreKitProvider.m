//
//  MPFeatureDetector.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPStoreKitProvider.h"
#import "MPGlobal.h"

#import <StoreKit/StoreKit.h>

/*
 * On iOS 7 and above, SKStoreProductViewController can cause a crash if the application does not list Portrait as a supported
 * interface orientation. Specifically, SKStoreProductViewController's shouldAutorotate returns YES, even though
 * the SKStoreProductViewController's supported interface orientations does not intersect with the application's list.
 *
 * To fix, we disallow autorotation so the SKStoreProductViewController will use its supported orientation on iOS 7 devices.
 */
@interface MPiOS7SafeStoreProductViewController : SKStoreProductViewController

@end

@implementation MPiOS7SafeStoreProductViewController

- (BOOL)shouldAutorotate
{
    return NO;
}

@end

@implementation MPStoreKitProvider

+ (BOOL)deviceHasStoreKit
{
    return !!NSClassFromString(@"SKStoreProductViewController");
}

+ (SKStoreProductViewController *)buildController
{
    // use our safe subclass on iOS 7 and above
    if ([[UIDevice currentDevice].systemVersion compare:@"7.0" options:NSNumericSearch] != NSOrderedAscending) {
        return [[MPiOS7SafeStoreProductViewController alloc] init];
    } else {
        return [[SKStoreProductViewController alloc] init];
    }
}

@end
