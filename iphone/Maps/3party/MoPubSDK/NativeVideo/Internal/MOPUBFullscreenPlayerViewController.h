//
//  MOPUBFullscreenPlayerViewController.h
//  Copyright (c) 2015 MoPub. All rights reserved.
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

- (instancetype)initWithVideoPlayer:(MOPUBPlayerViewController *)playerController dismissBlock:(MOPUBFullScreenPlayerViewControllerDismissBlock)dismiss;

@end
