//
//  MPLastResortDelegate.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <EventKitUI/EventKitUI.h>
#import <StoreKit/StoreKit.h>

@interface MPLastResortDelegate : NSObject<EKEventEditViewDelegate
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
, SKStoreProductViewControllerDelegate
#endif
>

+ (id)sharedDelegate;

@end
