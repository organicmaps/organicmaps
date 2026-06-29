#import "MWMAlert+CPP.h"
#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMEditorViralAlert.h"
#import "MWMLocationAlert.h"
#import "MWMOsmAuthAlert.h"
#import "MWMOsmReauthAlert.h"
#import "MWMPlaceDoesntExistAlert.h"
#import "MWMRoutingDisclaimerAlert.h"

#import "SwiftBridge.h"

@implementation MWMAlert

+ (MWMAlert *)locationAlertWithCancelBlock:(MWMVoidBlock)cancelBlock
{
  return [MWMLocationAlert alertWithCancelBlock:cancelBlock];
}

+ (MWMAlert *)routingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block
{
  return [MWMRoutingDisclaimerAlert alertWithOkBlock:block];
}

+ (MWMAlert *)locationServicesDisabledAlert
{
  return [LocationServicesDisabledAlert alert];
}

+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::CountriesSet const &)countries
                                            code:(routing::RouterResultCode)code
                                     cancelBlock:(MWMVoidBlock)cancelBlock
                                   downloadBlock:(MWMDownloadBlock)downloadBlock
                           downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock
{
  return [MWMDownloadTransitMapAlert downloaderAlertWithMaps:countries
                                                        code:code
                                                 cancelBlock:cancelBlock
                                               downloadBlock:downloadBlock
                                       downloadCompleteBlock:downloadCompleteBlock];
}

+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block
{
  return [MWMPlaceDoesntExistAlert alertWithBlock:block];
}

+ (MWMAlert *)editorViralAlert
{
  return [MWMEditorViralAlert alert];
}
+ (MWMAlert *)osmAuthAlert
{
  return [MWMOsmAuthAlert alert];
}
+ (MWMAlert *)osmReauthAlert
{
  return [MWMOsmReauthAlert alert];
}
+ (MWMAlert *)createBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                             minCharacterNum:(NSUInteger)min
                                                    callback:(MWMCheckStringBlock)callback
{
  return [MWMBCCreateCategoryAlert alertWithMaxCharachersNum:max minCharactersNum:min callback:callback];
}

+ (MWMAlert *)spinnerAlertWithTitle:(NSString *)title cancel:(MWMVoidBlock)cancel
{
  return [MWMSpinnerAlert alertWithTitle:title cancel:cancel];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  // Should override this method if you want custom relayout after rotation.
}

- (void)close:(MWMVoidBlock)completion
{
  [self.alertController closeAlert:completion];
}
- (void)setNeedsCloseAlertAfterEnterBackground
{
  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(applicationDidEnterBackground)
                                             name:UIApplicationDidEnterBackgroundNotification
                                           object:nil];
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
}
- (void)applicationDidEnterBackground
{
  // Should close alert when application entered background.
  [self close:nil];
}

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  if ([self respondsToSelector:@selector(willRotateToInterfaceOrientation:)])
    [self willRotateToInterfaceOrientation:toInterfaceOrientation];
}

- (void)addControllerViewToWindow
{
  UIWindow * window = UIApplication.sharedApplication.delegate.window;
  UIView * view = self.alertController.view;
  [window addSubview:view];
  view.frame = window.bounds;
}

- (void)setAlertController:(MWMAlertViewController *)alertController
{
  _alertController = alertController;
  UIView * view = alertController.view;
  UIViewController * ownerViewController = alertController.ownerViewController;
  view.frame = ownerViewController.view.bounds;
  [ownerViewController.view addSubview:view];
  [self addControllerViewToWindow];
  auto const orientation = view.window.windowScene.interfaceOrientation;
  [self rotate:orientation duration:0.0];
  [view addSubview:self];
  self.frame = view.bounds;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.frame = self.superview.bounds;
  [super layoutSubviews];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle)
    [self updateViewStyle:self];
}

- (void)updateViewStyle:(UIView *)view
{
  if (!view)
    return;
  for (UIView * subview in view.subviews)
    [self updateViewStyle:subview];
  [view applyTheme];
}

@end
