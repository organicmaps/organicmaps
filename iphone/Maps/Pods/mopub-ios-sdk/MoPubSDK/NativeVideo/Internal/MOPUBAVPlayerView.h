//
//  MOPUBAVPlayerView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@class AVPlayer;

@interface MOPUBAVPlayerView : UIView

@property (nonatomic) AVPlayer *player;
@property (nonatomic) NSString *videoGravity;

@end
