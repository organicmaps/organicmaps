//
//  MPAVPlayerView.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import "MOPUBAVPlayerView.h"

@implementation MOPUBAVPlayerView

+ (Class)layerClass
{
    return [AVPlayerLayer class];
}

- (AVPlayer *)player
{
    AVPlayerLayer *playerLayer = (AVPlayerLayer *)self.layer;
    return playerLayer.player;
}

- (void)setPlayer:(AVPlayer *)player
{
    AVPlayerLayer *playerLayer = (AVPlayerLayer *)self.layer;
    playerLayer.player = player;
}

- (NSString *)videoGravity
{
    AVPlayerLayer *playerLayer = (AVPlayerLayer *)self.layer;
    return playerLayer.videoGravity;
}

- (void)setVideoGravity:(NSString *)videoGravity
{
    AVPlayerLayer *playerLayer = (AVPlayerLayer *)self.layer;
    playerLayer.videoGravity = videoGravity;
}

@end
