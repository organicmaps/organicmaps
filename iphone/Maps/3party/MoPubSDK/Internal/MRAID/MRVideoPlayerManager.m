//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MRVideoPlayerManager.h"
#import <MediaPlayer/MediaPlayer.h>
#import "MPInstanceProvider.h"

@implementation MRVideoPlayerManager

@synthesize delegate = _delegate;

- (id)initWithDelegate:(id<MRVideoPlayerManagerDelegate>)delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:MPMoviePlayerPlaybackDidFinishNotification
                                                  object:nil];
}

- (void)playVideo:(NSURL *)url
{
    if (!url) {
        [self.delegate videoPlayerManager:self didFailToPlayVideoWithErrorMessage:@"URI was not valid."];
        return;
    }

    MPMoviePlayerViewController *controller = [[MPInstanceProvider sharedProvider] buildMPMoviePlayerViewControllerWithURL:url];

    [self.delegate videoPlayerManagerWillPresentVideo:self];
    [[self.delegate viewControllerForPresentingVideoPlayer] presentViewController:controller animated:MP_ANIMATED completion:nil];

    // Avoid subscribing to the notification multiple times in the event the user plays the video more than once.
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:MPMoviePlayerPlaybackDidFinishNotification
                                                  object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayerPlaybackDidFinish:)
                                                 name:MPMoviePlayerPlaybackDidFinishNotification
                                               object:nil];
}

- (void)moviePlayerPlaybackDidFinish:(NSNotification *)notification
{
    [self.delegate videoPlayerManagerDidDismissVideo:self];
}

@end
