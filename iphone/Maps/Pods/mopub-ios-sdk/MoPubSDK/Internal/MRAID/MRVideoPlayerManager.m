//
//  MRVideoPlayerManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <AVKit/AVKit.h>
#import <MediaPlayer/MediaPlayer.h>
#import "MPGlobal.h"
#import "MRVideoPlayerManager.h"

@interface MRVideoPlayerManager ()

@property (nonatomic, strong) AVPlayerViewController *playerViewController;

@end

@implementation MRVideoPlayerManager

- (id)initWithDelegate:(id<MRVideoPlayerManagerDelegate>)delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(moviePlayerPlaybackDidFinish:)
                                                     name:AVPlayerItemDidPlayToEndTimeNotification
                                                   object:nil];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)playVideo:(NSURL *)url
{
    if (!url) {
        [self.delegate videoPlayerManager:self didFailToPlayVideoWithErrorMessage:@"URI was not valid."];
        return;
    }

    AVPlayerViewController *viewController = [AVPlayerViewController new];
    viewController.player = [AVPlayer playerWithURL:url];
    viewController.showsPlaybackControls = NO;
    self.playerViewController = viewController;
    [self.delegate videoPlayerManagerWillPresentVideo:self];
    [[self.delegate viewControllerForPresentingVideoPlayer] presentViewController:viewController
                                                                         animated:MP_ANIMATED
                                                                       completion:^{
                                                                           [viewController.player play];
                                                                       }];
}

- (void)moviePlayerPlaybackDidFinish:(NSNotification *)notification
{
    [self.playerViewController dismissViewControllerAnimated:YES completion:^{
        [self.delegate videoPlayerManagerDidDismissVideo:self];
    }];
}

@end
