//
//  MOPUBPlayerManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBPlayerManager.h"
#import "MOPUBPlayerViewController.h"
#import "MPVideoConfig.h"
#import "MOPUBNativeVideoAdConfigValues.h"

@interface MOPUBPlayerManager()

@property (nonatomic) MOPUBPlayerViewController *currentPlayerViewController;

@end

@implementation MOPUBPlayerManager

+ (MOPUBPlayerManager *)sharedInstance
{
    static MOPUBPlayerManager *sharedInstance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[MOPUBPlayerManager alloc] init];
    });
    return sharedInstance;
}

- (void)disposePlayerViewController
{
    [self.currentPlayerViewController dispose];
    self.currentPlayerViewController = nil;
}

- (MOPUBPlayerViewController *)playerViewControllerWithVideoConfig:(MPVideoConfig *)videoConfig nativeVideoAdConfig:(MOPUBNativeVideoAdConfigValues *)nativeVideoAdConfig
{
    // make sure only one instance of avPlayer at a time
    if (self.currentPlayerViewController) {
        [self disposePlayerViewController];
    }

    self.currentPlayerViewController = [[MOPUBPlayerViewController alloc] initWithVideoConfig:videoConfig nativeVideoAdConfig:nativeVideoAdConfig];
    return self.currentPlayerViewController;
}

@end
