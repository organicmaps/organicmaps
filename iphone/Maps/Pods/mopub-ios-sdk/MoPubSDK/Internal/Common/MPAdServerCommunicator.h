//
//  MPAdServerCommunicator.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

#import "MPAdConfiguration.h"
#import "MPGlobal.h"

@protocol MPAdServerCommunicatorDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdServerCommunicator : NSObject

@property (nonatomic, weak) id<MPAdServerCommunicatorDelegate> delegate;
@property (nonatomic, assign, readonly) BOOL loading;

- (id)initWithDelegate:(id<MPAdServerCommunicatorDelegate>)delegate;

- (void)loadURL:(NSURL *)URL;
- (void)cancel;

- (void)sendBeforeLoadUrlWithConfiguration:(MPAdConfiguration *)configuration;
- (void)sendAfterLoadUrlWithConfiguration:(MPAdConfiguration *)configuration
                      adapterLoadDuration:(NSTimeInterval)duration
                        adapterLoadResult:(MPAfterLoadResult)result;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPAdServerCommunicatorDelegate <NSObject>

@required
- (void)communicatorDidReceiveAdConfigurations:(NSArray<MPAdConfiguration *> *)configurations;
- (void)communicatorDidFailWithError:(NSError *)error;

- (BOOL)isFullscreenAd;
- (NSString *)adUnitId;
@end
