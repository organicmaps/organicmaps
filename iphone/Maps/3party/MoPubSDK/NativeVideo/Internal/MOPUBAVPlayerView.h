//
//  MPAVPlayerView.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@class AVPlayer;

@interface MOPUBAVPlayerView : UIView

@property (nonatomic) AVPlayer *player;
@property (nonatomic) NSString *videoGravity;

@end
