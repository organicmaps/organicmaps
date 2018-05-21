//
//  MPFeatureDetector.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPGlobal.h"
#import <StoreKit/StoreKit.h>

@class SKStoreProductViewController;

@interface MPStoreKitProvider : NSObject

+ (BOOL)deviceHasStoreKit;
+ (SKStoreProductViewController *)buildController;

@end

@protocol MPSKStoreProductViewControllerDelegate <SKStoreProductViewControllerDelegate>
@end
