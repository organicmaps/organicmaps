//
//  MPAdWebViewAgent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPAdDestinationDisplayAgent.h"
#import "MPWebView.h"

enum {
    MPAdWebViewEventAdDidAppear     = 0,
    MPAdWebViewEventAdDidDisappear  = 1
};
typedef NSUInteger MPAdWebViewEvent;

@protocol MPAdWebViewAgentDelegate;

@class MPAdConfiguration;
@class CLLocation;
@class MPViewabilityTracker;

@interface MPAdWebViewAgent : NSObject <MPWebViewDelegate, MPAdDestinationDisplayAgentDelegate>

@property (nonatomic, strong) MPWebView *view;
@property (nonatomic, weak) id<MPAdWebViewAgentDelegate> delegate;

@property (nonatomic, strong, readonly) MPViewabilityTracker *viewabilityTracker;

- (id)initWithAdWebViewFrame:(CGRect)frame delegate:(id<MPAdWebViewAgentDelegate>)delegate;
- (void)loadConfiguration:(MPAdConfiguration *)configuration;
- (void)invokeJavaScriptForEvent:(MPAdWebViewEvent)event;

- (void)enableRequestHandling;
- (void)disableRequestHandling;

- (void)startViewabilityTracker;

@end

@protocol MPAdWebViewAgentDelegate <NSObject>

- (UIViewController *)viewControllerForPresentingModalView;
- (void)adDidClose:(MPWebView *)ad;
- (void)adDidFinishLoadingAd:(MPWebView *)ad;
- (void)adDidFailToLoadAd:(MPWebView *)ad;
- (void)adActionWillBegin:(MPWebView *)ad;
- (void)adActionWillLeaveApplication:(MPWebView *)ad;
- (void)adActionDidFinish:(MPWebView *)ad;

@end
