#import "Common.h"
#import "MWMBasePlacePageView.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"
#import <objc/runtime.h>

static NSString * const kPlacePageNavigationBarNibName = @"PlacePageNavigationBar";
static CGFloat const kNavigationBarHeight = 64.;

static inline CGPoint const openCenter(CGFloat xPosition)
{
  return CGPointMake(xPosition, kNavigationBarHeight / 2.);
}

static inline CGPoint const dismissCenter(CGFloat xPosition)
{
  return CGPointMake(xPosition, - kNavigationBarHeight / 2.);
}

@interface MWMPlacePageNavigationBar ()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) MWMiPhonePortraitPlacePage * placePage;

@end

@implementation MWMPlacePageNavigationBar

+ (void)remove
{
  UIScreen * screen = [UIScreen mainScreen];
  MWMPlacePageNavigationBar * navBar = objc_getAssociatedObject(screen, @selector(navigationBarWithPlacePage:));
  if (!navBar)
    return;

  [navBar removeFromSuperview];
  objc_setAssociatedObject(screen, @selector(navigationBarWithPlacePage:), nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

+ (void)showNavigationBarForPlacePage:(MWMiPhonePortraitPlacePage *)placePage
{
  UIView const * superview = placePage.manager.ownerViewController.view;
  UIScreen * screen = [UIScreen mainScreen];
  MWMPlacePageNavigationBar * navBar = objc_getAssociatedObject(screen, @selector(navigationBarWithPlacePage:));
  if (!navBar)
  {
    navBar = [self navigationBarWithPlacePage:placePage];
    objc_setAssociatedObject(screen, @selector(navigationBarWithPlacePage:), navBar, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [superview addSubview:navBar];
  }

  navBar.placePage = placePage;
  MWMPlacePageEntity * entity = placePage.manager.entity;
  navBar.titleLabel.text = entity.isMyPosition ? entity.bookmarkTitle : entity.title;
  [navBar show];
}

+ (void)dismissNavigationBar
{
  UIScreen * screen = [UIScreen mainScreen];
  MWMPlacePageNavigationBar * navBar = objc_getAssociatedObject(screen, @selector(navigationBarWithPlacePage:));
  if (!navBar)
    return;

  [navBar dismiss];
}

- (void)dismiss
{
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.center = dismissCenter(self.center.x);
  }];
}

- (void)show
{
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.center = openCenter(self.center.x);
  }];
}

+ (instancetype)navigationBarWithPlacePage:(MWMiPhonePortraitPlacePage *)placePage
{
  MWMPlacePageNavigationBar * navBar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageNavigationBarNibName owner:nil options:nil] firstObject];
  navBar.placePage = placePage;
  navBar.autoresizingMask = UIViewAutoresizingNone;
  UIView const * superview = placePage.manager.ownerViewController.view;
  navBar.center = dismissCenter(superview.center.x);
  CGSize const size = [[UIScreen mainScreen] bounds].size;
  BOOL const isLandscape = size.width > size.height;
  CGFloat const width = isLandscape ? size.height : size.width;
  navBar.width = width;
  return navBar;
}

- (IBAction)backTap:(id)sender
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBack)];
  [self dismiss];
  [self.placePage.manager refreshPlacePage];
}

- (void)layoutSubviews
{
  if (self)
    self.origin = CGPointZero;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
// Prevent super call to stop event propagation
// [super touchesBegan:touches withEvent:event];
}

@end
