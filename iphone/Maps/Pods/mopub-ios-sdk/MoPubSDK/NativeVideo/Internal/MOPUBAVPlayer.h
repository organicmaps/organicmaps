//
//  MOPUBAVPlayer.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <AVFoundation/AVFoundation.h>

@class MOPUBAVPlayer;

@protocol MOPUBAVPlayerDelegate <NSObject>

- (void)avPlayer:(MOPUBAVPlayer *)player didError:(NSError *)error withMessage:(NSString *)message;

- (void)avPlayer:(MOPUBAVPlayer *)player playbackTimeDidProgress:(NSTimeInterval)currentPlaybackTime;

- (void)avPlayerDidFinishPlayback:(MOPUBAVPlayer *)player;

- (void)avPlayerDidRecoverFromStall:(MOPUBAVPlayer *)player;

- (void)avPlayerDidStall:(MOPUBAVPlayer *)player;

@end


@interface MOPUBAVPlayer : AVPlayer

// Indicates the duration of the player item.
@property (nonatomic, readonly) NSTimeInterval currentItemDuration;

// Returns the current time of the current player item.
@property (nonatomic, readonly) NSTimeInterval currentPlaybackTime;

- (id)initWithDelegate:(id<MOPUBAVPlayerDelegate>)delegate playerItem:(AVPlayerItem *)playerItem;

- (void)dispose;

@end
