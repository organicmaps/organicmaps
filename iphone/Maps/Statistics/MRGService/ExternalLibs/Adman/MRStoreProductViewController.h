//
//  MRStoreProductViewController.h
//  MRAdMan
//
//  Created by Пучка Илья on 19.02.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <StoreKit/SKStoreProductViewController.h>
#import "SKStoreProductViewControllerDelegateExtended.h"

@interface MRStoreProductViewController : SKStoreProductViewController

@property(nonatomic, assign) id <SKStoreProductViewControllerDelegateExtended> delegate;
@property (nonatomic) BOOL hideStatusBarInFullScreen;

@end
