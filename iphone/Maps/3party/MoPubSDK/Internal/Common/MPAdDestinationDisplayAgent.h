//
//  MPAdDestinationDisplayAgent.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPActivityViewControllerHelper+TweetShare.h"
#import "MPURLResolver.h"
#import "MPProgressOverlayView.h"
#import "MPAdBrowserController.h"
#import "MPStoreKitProvider.h"

@protocol MPAdDestinationDisplayAgentDelegate;

@interface MPAdDestinationDisplayAgent : NSObject <MPProgressOverlayViewDelegate,
                                                   MPAdBrowserControllerDelegate,
                                                   MPSKStoreProductViewControllerDelegate,
                                                   MPActivityViewControllerHelperDelegate>

@property (nonatomic, weak) id<MPAdDestinationDisplayAgentDelegate> delegate;

+ (MPAdDestinationDisplayAgent *)agentWithDelegate:(id<MPAdDestinationDisplayAgentDelegate>)delegate;
- (void)displayDestinationForURL:(NSURL *)URL;
- (void)cancel;

@end

@protocol MPAdDestinationDisplayAgentDelegate <NSObject>

- (UIViewController *)viewControllerForPresentingModalView;
- (void)displayAgentWillPresentModal;
- (void)displayAgentWillLeaveApplication;
- (void)displayAgentDidDismissModal;

@optional

- (MPAdConfiguration *)adConfiguration;

@end
