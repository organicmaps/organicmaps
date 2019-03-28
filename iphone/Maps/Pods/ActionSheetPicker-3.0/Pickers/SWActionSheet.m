//
// Created by Petr Korolev on 11/08/14.
//

#import "SWActionSheet.h"


static const float delay = 0.f;

static const float duration = .25f;

static const enum UIViewAnimationOptions options = UIViewAnimationOptionCurveEaseIn;


@interface SWActionSheetVC : UIViewController

@property (nonatomic, retain) SWActionSheet *actionSheet;

@end


@interface SWActionSheet ()
{
    UIWindow *SWActionSheetWindow;
}

@property (nonatomic, assign) BOOL presented;
@property (nonatomic) UIWindowLevel windowLevel;

- (void)configureFrameForBounds:(CGRect)bounds;
- (void)showInContainerViewAnimated:(BOOL)animated;

@end


@implementation SWActionSheet
{
    UIView *view;
    UIView *_bgView;
}

- (void)dismissWithClickedButtonIndex:(int)i animated:(BOOL)animated
{
    CGPoint fadeOutToPoint = CGPointMake(view.center.x,
            self.center.y + CGRectGetHeight(view.frame));
    // Window of app
    //UIWindow *appWindow = [UIApplication sharedApplication].windows.firstObject;
    // Actions
    void (^actions)(void) = ^{
        self.center = fadeOutToPoint;
        self.backgroundColor = [UIColor colorWithWhite:0.f alpha:0.0f];
    };
    void (^completion)(BOOL) = ^(BOOL finished) {
    //    if (![appWindow isKeyWindow])
    //        [appWindow makeKeyAndVisible];
        [self destroyWindow];
        [self removeFromSuperview];
    };
    // Do actions animated or not
    if (animated) {
        [UIView animateWithDuration:duration delay:delay options:options animations:actions completion:completion];
    } else {
        actions();
        completion(YES);
    }
    self.presented = NO;
}

- (void)destroyWindow
{
    if (SWActionSheetWindow)
    {
        [self actionSheetContainer].actionSheet = nil;
        SWActionSheetWindow.hidden = YES;
        if ([SWActionSheetWindow isKeyWindow])
            [SWActionSheetWindow resignFirstResponder];
        SWActionSheetWindow.rootViewController = nil;
        SWActionSheetWindow = nil;
    }
}

- (UIWindow *)window
{
    if ( SWActionSheetWindow )
    {
        return SWActionSheetWindow;
    }
    else
    {
        return SWActionSheetWindow = ({
            UIWindow *window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
            window.windowLevel        = self.windowLevel;
            window.backgroundColor    = [UIColor clearColor];
            window.rootViewController = [SWActionSheetVC new];
            window;
        });
    }
}

- (SWActionSheetVC *)actionSheetContainer
{
    return (SWActionSheetVC *) [self window].rootViewController;
}

- (instancetype)initWithView:(UIView *)aView windowLevel:(UIWindowLevel)windowLevel
{
    if ((self = [super init]))
    {
        view = aView;
        _windowLevel = windowLevel;
        self.backgroundColor = [UIColor colorWithWhite:0.f alpha:0.0f];
        _bgView = [UIView new];
        _bgView.backgroundColor = [UIColor colorWithRed:247.f/255.f green:247.f/255.f blue:247.f/255.f alpha:1.0f];
        [self addSubview:_bgView];
        [self addSubview:view];
    }
    return self;
}

- (void)configureFrameForBounds:(CGRect)bounds
{
    self.frame = CGRectMake(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height + view.bounds.size.height);
    view.frame = CGRectMake(view.bounds.origin.x, bounds.size.height, view.bounds.size.width, view.bounds.size.height);
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    _bgView.frame = view.frame;
    _bgView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)showFromBarButtonItem:(UIBarButtonItem *)item animated:(BOOL)animated
{
    [self showInContainerView];
}

- (void)showInContainerView
{
    // Make sheet window visible and active
    UIWindow *sheetWindow = [self window];
    if (![sheetWindow isKeyWindow])
        [sheetWindow makeKeyAndVisible];
    sheetWindow.hidden = NO;
    // Put our ActionSheet in Container (it will be presented as soon as possible)
    self.actionSheetContainer.actionSheet = self;
}

- (void)showInContainerViewAnimated:(BOOL)animated
{
    CGPoint toPoint;
    CGFloat y = self.center.y - CGRectGetHeight(view.frame);
    toPoint = CGPointMake(self.center.x, y);
    // Present actions
    void (^animations)(void) = ^{
        self.center = toPoint;
        self.backgroundColor = [UIColor colorWithWhite:0.f alpha:0.5f];
    };
    // Present sheet
    if (animated)
        [UIView animateWithDuration:duration delay:delay options:options animations:animations completion:nil];
    else
        animations();
    self.presented = YES;
}

@end


#pragma mark - SWActionSheet Container

@implementation SWActionSheetVC


- (UIStatusBarStyle)preferredStatusBarStyle {
    return [UIApplication sharedApplication].statusBarStyle;
}

- (void)setActionSheet:(SWActionSheet *)actionSheet
{
    // Prevent processing one action sheet twice
    if (_actionSheet == actionSheet)
        return;
    // Dissmiss previous action sheet if it presented
    if (_actionSheet.presented)
        [_actionSheet dismissWithClickedButtonIndex:0 animated:YES];
    // Remember new action sheet
    _actionSheet = actionSheet;
    // Present new action sheet
    [self presentActionSheetAnimated:YES];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self presentActionSheetAnimated:YES];
}

- (void)presentActionSheetAnimated:(BOOL)animated
{
    // New action sheet will be presented only when view controller will be loaded
    if (_actionSheet && [self isViewLoaded] && !_actionSheet.presented)
    {
        [_actionSheet configureFrameForBounds:self.view.bounds];
        _actionSheet.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [self.view addSubview:_actionSheet];
        [_actionSheet showInContainerViewAnimated:animated];
    }
}

- (BOOL)prefersStatusBarHidden {
	return [UIApplication sharedApplication].statusBarHidden;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return NO;
}

// iOS6 support
// ---
- (BOOL)shouldAutorotate
{
    return YES;
}

@end
