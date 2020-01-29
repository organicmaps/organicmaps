//
//  MPAdDestinationDisplayAgent.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPActivityViewControllerHelper+TweetShare.h"
#import "MPURLResolver.h"
#import "MPProgressOverlayView.h"
#import "MOPUBDisplayAgentType.h"

@protocol MPAdDestinationDisplayAgentDelegate;

@interface MPAdDestinationDisplayAgent : NSObject <MPProgressOverlayViewDelegate,
                                                   MPActivityViewControllerHelperDelegate>

@property (nonatomic, weak) id<MPAdDestinationDisplayAgentDelegate> delegate;

+ (MPAdDestinationDisplayAgent *)agentWithDelegate:(id<MPAdDestinationDisplayAgentDelegate>)delegate;
+ (BOOL)shouldDisplayContentInApp;
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
