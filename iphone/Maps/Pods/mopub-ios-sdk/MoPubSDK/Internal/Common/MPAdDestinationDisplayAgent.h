//
//  MPAdDestinationDisplayAgent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPActivityViewControllerHelper+TweetShare.h"
#import "MPAdConfiguration.h"
#import "MPURLResolver.h"
#import "MPProgressOverlayView.h"
#import "MOPUBDisplayAgentType.h"

@protocol MPAdDestinationDisplayAgentDelegate;

@protocol MPAdDestinationDisplayAgent

@property (nonatomic, weak) id<MPAdDestinationDisplayAgentDelegate> delegate;

+ (id<MPAdDestinationDisplayAgent>)agentWithDelegate:(id<MPAdDestinationDisplayAgentDelegate>)delegate;
+ (BOOL)shouldDisplayContentInApp;
- (void)displayDestinationForURL:(NSURL *)URL;
- (void)cancel;

@end

@interface MPAdDestinationDisplayAgent : NSObject <
    MPAdDestinationDisplayAgent,
    MPProgressOverlayViewDelegate,
    MPActivityViewControllerHelperDelegate
>

@end

@protocol MPAdDestinationDisplayAgentDelegate <NSObject>

- (UIViewController *)viewControllerForPresentingModalView;
- (void)displayAgentWillPresentModal;
- (void)displayAgentWillLeaveApplication;
- (void)displayAgentDidDismissModal;

@optional

- (MPAdConfiguration *)adConfiguration;

@end
