//
//  MPLastResortDelegate.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

@interface MPLastResortDelegate : NSObject <SKStoreProductViewControllerDelegate>

+ (id)sharedDelegate;

@end
