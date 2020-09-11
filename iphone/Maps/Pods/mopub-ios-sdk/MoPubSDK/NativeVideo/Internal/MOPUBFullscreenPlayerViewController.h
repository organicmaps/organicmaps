//
//  MOPUBFullscreenPlayerViewController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@class MOPUBPlayerViewController;
@class MOPUBPlayerView;
@class MOPUBFullscreenPlayerViewController;

@protocol MOPUBFullscreenPlayerViewControllerDelegate <NSObject>

- (void)playerDidProgressToTime:(NSTimeInterval)playbackTime;
- (void)ctaTapped:(MOPUBFullscreenPlayerViewController *)viewController;
- (void)fullscreenPlayerWillLeaveApplication:(MOPUBFullscreenPlayerViewController *)viewController;

@end

typedef void (^MOPUBFullScreenPlayerViewControllerDismissBlock)(UIView *originalParentView);

@interface MOPUBFullscreenPlayerViewController : UIViewController

@property (nonatomic) MOPUBPlayerView *playerView;

@property (nonatomic, weak) id<MOPUBFullscreenPlayerViewControllerDelegate> delegate;

- (instancetype)initWithVideoPlayer:(MOPUBPlayerViewController *)playerController nativeAdProperties:(NSDictionary *)properties dismissBlock:(MOPUBFullScreenPlayerViewControllerDismissBlock)dismiss;

@end
