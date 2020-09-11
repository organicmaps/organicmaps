//
//  SKStoreProductViewController+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <StoreKit/StoreKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SKStoreProductViewController (MPAdditions)

/**
 @c SKStoreProductViewController can crash the app if used under the wrong conditions (e.g.,
 in the case of an orientation mismatch), so this property reports whether it's safe to use
 the view controller.
 */
@property (class, nonatomic, readonly) BOOL canUseStoreProductViewController;

@end

NS_ASSUME_NONNULL_END
