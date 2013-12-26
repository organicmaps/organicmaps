//
//  MPFeatureDetector.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPStoreKitProvider.h"
#import "MPGlobal.h"

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
#import <StoreKit/StoreKit.h>
#endif

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
/*
 * On iOS 7 SKStoreProductViewController can cause a crash if the application does not list Portrait as a supported
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
#endif

@implementation MPStoreKitProvider

+ (BOOL)deviceHasStoreKit
{
    return !!NSClassFromString(@"SKStoreProductViewController");
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
+ (SKStoreProductViewController *)buildController
{
    // use our safe subclass on iOS 7
    if([[UIDevice currentDevice].systemVersion compare:@"7.0"] != NSOrderedAscending)
    {
        return [[[MPiOS7SafeStoreProductViewController alloc] init] autorelease];
    }
    else
    {
        return [[[SKStoreProductViewController alloc] init] autorelease];
    }
}
#endif

@end
