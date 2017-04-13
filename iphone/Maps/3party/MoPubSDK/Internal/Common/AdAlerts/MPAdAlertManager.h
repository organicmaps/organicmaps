//
//  MPAdAlertManager.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPGlobal.h"

@class CLLocation;
@protocol MPAdAlertManagerDelegate;

@class MPAdConfiguration;

@interface MPAdAlertManager : NSObject <MPAdAlertManagerProtocol>

@end

@protocol MPAdAlertManagerDelegate <NSObject>

@required
- (UIViewController *)viewControllerForPresentingMailVC;
- (void)adAlertManagerDidTriggerAlert:(MPAdAlertManager *)manager;

@optional
- (void)adAlertManagerDidProcessAlert:(MPAdAlertManager *)manager;

@end