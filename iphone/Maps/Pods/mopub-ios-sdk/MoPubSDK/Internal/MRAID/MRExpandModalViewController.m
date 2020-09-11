//
//  MRExpandModalViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MRExpandModalViewController.h"
#import "MPGlobal.h"

@interface MRExpandModalViewController ()

@property (nonatomic, assign) UIInterfaceOrientationMask supportedOrientationMask;

@end

@implementation MRExpandModalViewController

- (instancetype)initWithOrientationMask:(UIInterfaceOrientationMask)orientationMask
{
    if (self = [super init]) {
        _supportedOrientationMask = orientationMask;
        self.modalPresentationStyle = UIModalPresentationFullScreen;
    }

    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)setSupportedOrientationMask:(UIInterfaceOrientationMask)supportedOrientationMask
{
    _supportedOrientationMask = supportedOrientationMask;

    [UIViewController attemptRotationToDeviceOrientation];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return ([[UIApplication sharedApplication] mp_supportsOrientationMask:self.supportedOrientationMask]) ? self.supportedOrientationMask : UIInterfaceOrientationMaskAll;
}

- (BOOL)shouldAutorotate
{
    return YES;
}

#pragma mark - <MPClosableViewDelegate>

// We transfer closable view delegation to the expand view controller in the event MRController is deallocated and the expand modal is presented.
- (void)closeButtonPressed:(MPClosableView *)closableView
{
    // All we need to do is dismiss ourself.
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
