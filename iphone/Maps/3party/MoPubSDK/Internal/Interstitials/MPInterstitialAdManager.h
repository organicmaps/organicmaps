//
//  MPInterstitialAdManager.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPAdServerCommunicator.h"
#import "MPBaseInterstitialAdapter.h"

@class CLLocation;
@protocol MPInterstitialAdManagerDelegate;

@interface MPInterstitialAdManager : NSObject <MPAdServerCommunicatorDelegate,
    MPInterstitialAdapterDelegate>

@property (nonatomic, weak) id<MPInterstitialAdManagerDelegate> delegate;
@property (nonatomic, assign, readonly) BOOL ready;

- (id)initWithDelegate:(id<MPInterstitialAdManagerDelegate>)delegate;

- (void)loadInterstitialWithAdUnitID:(NSString *)ID
                            keywords:(NSString *)keywords
                    userDataKeywords:(NSString *)userDataKeywords
                            location:(CLLocation *)location;
- (void)presentInterstitialFromViewController:(UIViewController *)controller;

@end
