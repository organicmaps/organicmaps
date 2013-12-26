//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MRVideoPlayerManager.h"
#import <MediaPlayer/MediaPlayer.h>
#import "MPInstanceProvider.h"
#import "UIViewController+MPAdditions.h"

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

    [super dealloc];
}

- (void)playVideo:(NSDictionary *)parameters
{
    NSString *URLString = [[parameters objectForKey:@"uri"] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    NSURL *URL = [NSURL URLWithString:URLString];

    if (!URL) {
        [self.delegate videoPlayerManager:self didFailToPlayVideoWithErrorMessage:@"URI was not valid."];
        return;
    }

    MPMoviePlayerViewController *controller = [[MPInstanceProvider sharedProvider] buildMPMoviePlayerViewControllerWithURL:URL];

    [self.delegate videoPlayerManagerWillPresentVideo:self];
    [[self.delegate viewControllerForPresentingVideoPlayer] mp_presentModalViewController:controller
                                                                                 animated:MP_ANIMATED];

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
