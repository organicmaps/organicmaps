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

@property (nonatomic, retain) MPAdWebViewAgent *backingViewAgent;
@property (nonatomic, assign) id customMethodDelegate;

- (void)loadConfiguration:(MPAdConfiguration *)configuration;

@end
