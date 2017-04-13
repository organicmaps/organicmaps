//
//  MOPUBPlayerManager.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBPlayerManager.h"
#import "MOPUBPlayerViewController.h"
#import "MOPUBNativeVideoAdRenderer.h"
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

- (MOPUBPlayerViewController *)playerViewControllerWithVideoConfig:(MPVideoConfig *)videoConfig nativeVideoAdConfig:(MOPUBNativeVideoAdConfigValues *)nativeVideoAdConfig logEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties
{
    // make sure only one instance of avPlayer at a time
    if (self.currentPlayerViewController) {
        [self disposePlayerViewController];
    }

    self.currentPlayerViewController = [[MOPUBPlayerViewController alloc] initWithVideoConfig:videoConfig nativeVideoAdConfig:nativeVideoAdConfig logEventProperties:logEventProperties];
    return self.currentPlayerViewController;
}

@end
