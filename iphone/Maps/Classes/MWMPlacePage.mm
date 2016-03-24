#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMBookmarkDescriptionViewController.h"
#import "MWMDirectionView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "SelectSetVC.h"
#import "Statistics.h"

static NSString * const kPlacePageNibIdentifier = @"PlacePageView";
static NSString * const kPlacePageViewCenterKeyPath = @"center";

@interface MWMPlacePage ()

@property (weak, nonatomic, readwrite) MWMPlacePageViewManager * manager;

@end

@implementation MWMPlacePage

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kPlacePageNibIdentifier owner:self options:nil];
    self.manager = manager;
    if (!IPAD)
    {
      [self.extendedPlacePageView addObserver:self
                                   forKeyPath:kPlacePageViewCenterKeyPath
                                      options:NSKeyValueObservingOptionNew
                                      context:nullptr];
    }
    dispatch_async(dispatch_get_main_queue(), ^
    {
      [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(keyboardWillShow:)
                                                   name:UIKeyboardWillShowNotification
                                                 object:nil];

      [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(keyboardWillHide)
                                                   name:UIKeyboardWillHideNotification
                                                 object:nil];
    });
  }
  return self;
}

- (void)dealloc
{
  if (!IPAD)
  {
    [self.extendedPlacePageView removeObserver:self forKeyPath:kPlacePageViewCenterKeyPath];
  }
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)keyboardWillShow:(NSNotification *)aNotification
{
  NSDictionary * info = [aNotification userInfo];
  self.keyboardHeight = [info[UIKeyboardFrameEndUserInfoKey] CGRectValue].size.height;
}

- (void)keyboardWillHide
{
  self.keyboardHeight = 0.0;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if ([self.extendedPlacePageView isEqual:object] &&
      [keyPath isEqualToString:kPlacePageViewCenterKeyPath])
    [self.manager dragPlacePage:self.extendedPlacePageView.frame];
}

- (void)configure
{
  MWMPlacePageEntity * entity = self.manager.entity;
  [self.basePlacePageView configureWithEntity:entity];
  BOOL const isPrepareRouteMode = MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  if (self.actionBar.isPrepareRouteMode == isPrepareRouteMode)
  {
    [self.actionBar configureWithPlacePage:self];
  }
  else
  {
    [self.actionBar removeFromSuperview];
    self.actionBar = [MWMPlacePageActionBar actionBarForPlacePage:self];
  }
}

- (void)show
{
  // Should override this method if you want to show place page with custom animation.
}

- (void)hide
{
  // Should override this method if you want to hide place page with custom animation.
}

- (void)dismiss
{
  [self.extendedPlacePageView removeFromSuperview];
  [self.actionBar removeFromSuperview];
  self.actionBar = nil;
}

#pragma mark - Actions
- (void)apiBack
{
  [self.manager apiBack];
}

- (void)addBookmark
{
  [self.manager addBookmark];
  [self.basePlacePageView addBookmark];
}

- (void)removeBookmark
{
  [self.manager removeBookmark];
  [self.basePlacePageView removeBookmark];
}

- (void)editPlace
{
  [self.manager editPlace];
}

- (void)reportProblem
{
  [self.manager reportProblem];
}

- (void)share
{
  [self.manager share];
}

- (void)route
{
  [self.manager buildRoute];
}

- (void)addPlacePageShadowToView:(UIView *)view offset:(CGSize)offset
{
  CALayer * layer = view.layer;
  layer.masksToBounds = NO;
  layer.shadowColor = UIColor.blackColor.CGColor;
  layer.shadowRadius = 4.;
  layer.shadowOpacity = 0.24f;
  layer.shadowOffset = offset;
  layer.shouldRasterize = YES;
  layer.rasterizationScale = [[UIScreen mainScreen] scale];
}

- (void)setDirectionArrowTransform:(CGAffineTransform)transform
{
  self.basePlacePageView.directionArrow.transform = transform;
}

- (void)setDistance:(NSString *)distance
{
  self.basePlacePageView.distanceLabel.text = distance;
}

- (void)updateMyPositionStatus:(NSString *)status
{
  [self.basePlacePageView updateAndLayoutMyPositionSpeedAndAltitude:status];
}

- (void)changeBookmarkCategory
{
  MWMPlacePageViewManager * manager = self.manager;
  MapViewController * ovc = static_cast<MapViewController *>(manager.ownerViewController);
  SelectSetVC * vc = [[SelectSetVC alloc] initWithPlacePageManager:manager];
  [ovc.navigationController pushViewController:vc animated:YES];
}

- (void)changeBookmarkColor
{
  MWMBookmarkColorViewController * controller = [[MWMBookmarkColorViewController alloc] initWithNibName:[MWMBookmarkColorViewController className] bundle:nil];
  controller.placePageManager = self.manager;
  [self.manager.ownerViewController.navigationController pushViewController:controller animated:YES];
}

- (void)changeBookmarkDescription
{
  MWMPlacePageViewManager * manager = self.manager;
  MapViewController * ovc = static_cast<MapViewController *>(manager.ownerViewController);
  MWMBookmarkDescriptionViewController * viewController = [[MWMBookmarkDescriptionViewController alloc] initWithPlacePageManager:manager];
  [ovc.navigationController pushViewController:viewController animated:YES];
}

- (void)reloadBookmark
{
  [self.basePlacePageView reloadBookmarkCell];
}

- (void)willStartEditingBookmarkTitle
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatRename)];
// This method should be оverridden.
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  self.basePlacePageView.titleLabel.text = title;
  [self.basePlacePageView layoutIfNeeded];
}

- (IBAction)didTap:(UITapGestureRecognizer *)sender
{
// This method should be оverridden if you want to process custom tap.
}

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  // This method should be оverridden if you want to process custom pan.
}

- (void)refresh
{
  // This method should be оverridden.
}

#pragma mark - Properties

- (MWMPlacePageActionBar *)actionBar
{
  if (!_actionBar)
    _actionBar = [MWMPlacePageActionBar actionBarForPlacePage:self];
  return _actionBar;
}

@end
