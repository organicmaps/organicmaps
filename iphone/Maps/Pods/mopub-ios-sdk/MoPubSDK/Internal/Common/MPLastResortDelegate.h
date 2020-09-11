//
//  MPLastResortDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

@interface MPLastResortDelegate : NSObject <SKStoreProductViewControllerDelegate>

+ (id)sharedDelegate;

@end
