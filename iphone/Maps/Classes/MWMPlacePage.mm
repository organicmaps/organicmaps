#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMDirectionView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"

static NSString * const kPlacePageNibIdentifier = @"PlacePageView";
static NSString * const kPlacePageViewCenterKeyPath = @"center";
extern NSString * const kPP2BookmarkEditingSegue = @"PP2BookmarkEditing";
extern NSString * const kPP2BookmarkEditingIPADSegue = @"PP2BookmarkEditingIPAD";

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
      [self.extendedPlacePageView addObserver:self
                                   forKeyPath:kPlacePageViewCenterKeyPath
                                      options:NSKeyValueObservingOptionNew
                                      context:nullptr];
  }
  return self;
}

- (void)dealloc
{
  if (!IPAD)
    [self.extendedPlacePageView removeObserver:self forKeyPath:kPlacePageViewCenterKeyPath];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if ([self.extendedPlacePageView isEqual:object] && [keyPath isEqualToString:kPlacePageViewCenterKeyPath])
    [self.manager dragPlacePage:self.extendedPlacePageView.frame];
}

- (void)configure
{
  MWMPlacePageEntity * entity = self.manager.entity;
  [self.basePlacePageView configureWithEntity:entity];
  BOOL const isPrepareRouteMode = MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  if (self.actionBar.isPrepareRouteMode == isPrepareRouteMode)
  {
    [self.actionBar configureWithPlacePageManager:self.manager];
  }
  else
  {
    [self.actionBar removeFromSuperview];
    self.actionBar = [MWMPlacePageActionBar actionBarForPlacePageManager:self.manager];
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

- (void)addBookmark
{
  [self.basePlacePageView addBookmark];
  self.actionBar.isBookmark = YES;
}

- (void)removeBookmark
{
  [self.basePlacePageView removeBookmark];
  self.actionBar.isBookmark = NO;
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
  self.basePlacePageView.distance = distance;
}

- (void)updateMyPositionStatus:(NSString *)status
{
  [self.basePlacePageView updateAndLayoutMyPositionSpeedAndAltitude:status];
}

- (void)editBookmark
{
  [self.manager.ownerViewController performSegueWithIdentifier:IPAD ? kPP2BookmarkEditingIPADSegue :
                                                              kPP2BookmarkEditingSegue sender:self.manager];
}

- (void)reloadBookmark
{
  [self.basePlacePageView reloadBookmarkCell];
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
    _actionBar = [MWMPlacePageActionBar actionBarForPlacePageManager:self.manager];
  return _actionBar;
}

@end
