//
//  MOPUBPlayerManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MOPUBPlayerViewController;
@class MOPUBNativeVideoAdConfigValues;
@class MPVideoConfig;

@interface MOPUBPlayerManager : NSObject

+ (MOPUBPlayerManager *)sharedInstance;
- (void)disposePlayerViewController;

- (MOPUBPlayerViewController *)playerViewControllerWithVideoConfig:(MPVideoConfig *)videoConfig nativeVideoAdConfig:(MOPUBNativeVideoAdConfigValues *)nativeVideoAdConfig;

@end
