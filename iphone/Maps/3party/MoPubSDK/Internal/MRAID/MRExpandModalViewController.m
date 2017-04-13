//
//  MRExpandModalViewController.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MRExpandModalViewController.h"
#import "MPGlobal.h"

@interface MRExpandModalViewController ()

@property (nonatomic, assign) BOOL statusBarHidden;
@property (nonatomic, assign) BOOL applicationHidesStatusBar;
@property (nonatomic, assign) UIInterfaceOrientationMask supportedOrientationMask;

@end

@implementation MRExpandModalViewController

- (instancetype)initWithOrientationMask:(UIInterfaceOrientationMask)orientationMask
{
    if (self = [super init]) {
        _supportedOrientationMask = orientationMask;
    }

    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
}

- (void)hideStatusBar
{
    if (!self.statusBarHidden) {
        self.statusBarHidden = YES;
        self.applicationHidesStatusBar = [UIApplication sharedApplication].statusBarHidden;

        // pre-ios 7 hiding status bar
        [[UIApplication sharedApplication] mp_preIOS7setApplicationStatusBarHidden:YES];

        // In the event we come back to this view controller from another modal, we need to update the status bar's
        // visibility again in ios 7/8.
        if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
            [self setNeedsStatusBarAppearanceUpdate];
        }
    }
}

- (void)restoreStatusBarVisibility
{
    self.statusBarHidden = self.applicationHidesStatusBar;

    // pre-ios 7 restoring the status bar
    [[UIApplication sharedApplication] mp_preIOS7setApplicationStatusBarHidden:self.applicationHidesStatusBar];

    // ios 7/8 restoring status bar
    if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
        [self setNeedsStatusBarAppearanceUpdate];
    }
}

- (BOOL)prefersStatusBarHidden
{
    // ios 7 hiding status bar
    return self.statusBarHidden;
}

- (void)setSupportedOrientationMask:(UIInterfaceOrientationMask)supportedOrientationMask
{
    _supportedOrientationMask = supportedOrientationMask;

    [UIViewController attemptRotationToDeviceOrientation];
}

// supportedInterfaceOrientations and shouldAutorotate are for ios 6, 7, and 8.
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_9_0
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
#else
- (NSUInteger)supportedInterfaceOrientations
#endif
{
    return ([[UIApplication sharedApplication] mp_supportsOrientationMask:self.supportedOrientationMask]) ? self.supportedOrientationMask : UIInterfaceOrientationMaskAll;
}

- (BOOL)shouldAutorotate
{
    return YES;
}

// shouldAutorotateToInterfaceOrientation is for ios 5.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return [[UIApplication sharedApplication] mp_doesOrientation:interfaceOrientation matchOrientationMask:self.supportedOrientationMask];
}

#pragma mark - <MPClosableViewDelegate>

// We transfer closable view delegation to the expand view controller in the event MRController is deallocated and the expand modal is presented.
- (void)closeButtonPressed:(MPClosableView *)closableView
{
    // All we need to do is dismiss ourself.
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
