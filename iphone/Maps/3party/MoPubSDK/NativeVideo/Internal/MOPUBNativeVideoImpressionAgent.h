//
//  MOPUBNativeVideoImpressionAgent.h
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface MOPUBNativeVideoImpressionAgent : NSObject

- (instancetype)initWithVideoView:(UIView *)videoView requiredVisibilityPercentage:(CGFloat)visiblePercentage requiredPlaybackDuration:(NSTimeInterval)playbackDuration;

- (BOOL)shouldTrackImpressionWithCurrentPlaybackTime:(NSTimeInterval)currentPlaybackTime;

@end
