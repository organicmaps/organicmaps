//
//  MPHTMLInterstitialViewController.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MPAdWebViewAgent.h"
#import "MPInterstitialViewController.h"

@class MPAdConfiguration;

@interface MPHTMLInterstitialViewController : MPInterstitialViewController <MPAdWebViewAgentDelegate>

@property (nonatomic, strong) MPAdWebViewAgent *backingViewAgent;

- (void)loadConfiguration:(MPAdConfiguration *)configuration;

@end
