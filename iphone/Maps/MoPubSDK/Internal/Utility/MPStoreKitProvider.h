//
//  MPFeatureDetector.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPGlobal.h"
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
#import <StoreKit/StoreKit.h>
#endif

@class SKStoreProductViewController;

@interface MPStoreKitProvider : NSObject

+ (BOOL)deviceHasStoreKit;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
+ (SKStoreProductViewController *)buildController;
#endif

@end

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
@protocol MPSKStoreProductViewControllerDelegate <SKStoreProductViewControllerDelegate>
#else
@protocol MPSKStoreProductViewControllerDelegate
#endif
@end
