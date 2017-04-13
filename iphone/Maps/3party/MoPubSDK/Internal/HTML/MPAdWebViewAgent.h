//
//  MPAdWebViewAgent.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
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

@interface MPAdWebViewAgent : NSObject <MPWebViewDelegate, MPAdDestinationDisplayAgentDelegate>

@property (nonatomic, strong) MPWebView *view;
@property (nonatomic, weak) id<MPAdWebViewAgentDelegate> delegate;

- (id)initWithAdWebViewFrame:(CGRect)frame delegate:(id<MPAdWebViewAgentDelegate>)delegate;
- (void)loadConfiguration:(MPAdConfiguration *)configuration;
- (void)rotateToOrientation:(UIInterfaceOrientation)orientation;
- (void)invokeJavaScriptForEvent:(MPAdWebViewEvent)event;
- (void)forceRedraw;

- (void)enableRequestHandling;
- (void)disableRequestHandling;

@end

@protocol MPAdWebViewAgentDelegate <NSObject>

- (NSString *)adUnitId;
- (CLLocation *)location;
- (UIViewController *)viewControllerForPresentingModalView;
- (void)adDidClose:(MPWebView *)ad;
- (void)adDidFinishLoadingAd:(MPWebView *)ad;
- (void)adDidFailToLoadAd:(MPWebView *)ad;
- (void)adActionWillBegin:(MPWebView *)ad;
- (void)adActionWillLeaveApplication:(MPWebView *)ad;
- (void)adActionDidFinish:(MPWebView *)ad;

@end
