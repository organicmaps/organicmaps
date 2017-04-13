//
//  MPPlayerView.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MOPUBAVPlayer.h"

@class MOPUBPlayerView;

typedef NS_ENUM(NSUInteger, MOPUBPlayerDisplayMode) {
    MOPUBPlayerDisplayModeInline = 0,
    MOPUBPlayerDisplayModeFullscreen
};

@protocol MOPUBPlayerViewDelegate <NSObject>

- (void)playerViewDidTapReplayButton:(MOPUBPlayerView *)view;
- (void)playerViewWillShowReplayView:(MOPUBPlayerView *)view;
- (void)playerViewWillEnterFullscreen:(MOPUBPlayerView *)view;

@end

@interface MOPUBPlayerView : UIControl

@property (nonatomic) MOPUBAVPlayer *avPlayer;
@property (nonatomic) MOPUBPlayerDisplayMode displayMode;
@property (nonatomic, copy) NSString *videoGravity;

- (instancetype)initWithFrame:(CGRect)frame delegate:(id<MOPUBPlayerViewDelegate>)delegate;

- (void)createPlayerView;
- (void)playbackTimeDidProgress;
- (void)playbackDidFinish;
- (void)setProgressBarVisible:(BOOL)visible;
- (void)handleVideoInitFailure;

@end
