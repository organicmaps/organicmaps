//
//  MPVideoPlayerViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTCompanionAd.h"
#import "MPVideoPlayerContainerView.h"
#import "MPVideoPlayerViewController.h"

@interface MPVideoPlayerViewController ()

@property (nonatomic, strong) MPVideoPlayerContainerView *videoPlayerContainerView;

@end

@interface MPVideoPlayerViewController (MPVASTResourceViewDelegate) <MPVASTResourceViewDelegate>
@end

@implementation MPVideoPlayerViewController

- (void)setDelegate:(id<MPVideoPlayerViewControllerDelegate>)delegate {
    _videoPlayerContainerView.delegate = delegate;
    _delegate = delegate;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

#pragma mark - MPVideoPlayer

- (instancetype)initWithVideoURL:(NSURL *)videoURL videoConfig:(MPVideoConfig *)videoConfig {
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        self.modalPresentationStyle = UIModalPresentationFullScreen;
        _videoPlayerContainerView = [[MPVideoPlayerContainerView alloc] initWithVideoURL:videoURL
                                                                             videoConfig:videoConfig];
    }
    return self;
}

- (void)loadVideo {
    [self.videoPlayerContainerView loadVideo];
}

- (void)play {
    [self.videoPlayerContainerView play];
}

- (void)pause {
    [self.videoPlayerContainerView pause];
}

- (void)enableAppLifeCycleEventObservationForAutoPlayPause {
    [self.videoPlayerContainerView enableAppLifeCycleEventObservationForAutoPlayPause];
}

- (void)disableAppLifeCycleEventObservationForAutoPlayPause {
    [self.videoPlayerContainerView disableAppLifeCycleEventObservationForAutoPlayPause];
}

#pragma mark - life cycle

- (void)loadView {
    // Use a `MPVideoPlayerView` instead of the plain `UIView` for `self.view`
    // Note: do not call `[super loadView]` as explained in Apple doc
    // https://developer.apple.com/documentation/uikit/uiviewcontroller/1621454-loadview
    self.view = self.videoPlayerContainerView;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self.delegate interstitialWillAppear:self];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self.delegate interstitialDidAppear:self];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self.delegate interstitialWillDisappear:self];
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    [self.delegate interstitialDidDisappear:self];
}

@end
