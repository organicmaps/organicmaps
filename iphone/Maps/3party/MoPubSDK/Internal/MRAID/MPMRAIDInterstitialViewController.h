//
//  MPMRAIDInterstitialViewController.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPInterstitialViewController.h"
#import "MPForceableOrientationProtocol.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPMRAIDInterstitialViewControllerDelegate;
@class MPAdConfiguration;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPMRAIDInterstitialViewController : MPInterstitialViewController <MPForceableOrientationProtocol>

- (id)initWithAdConfiguration:(MPAdConfiguration *)configuration;
- (void)startLoading;

@end

