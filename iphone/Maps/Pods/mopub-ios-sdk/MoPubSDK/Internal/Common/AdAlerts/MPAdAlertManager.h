//
//  MPAdAlertManager.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
