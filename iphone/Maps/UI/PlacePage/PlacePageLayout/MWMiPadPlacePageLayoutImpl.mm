#import "MWMiPadPlacePageLayoutImpl.h"
#import "MWMPlacePageLayout.h"
#import "SwiftBridge.h"

namespace
{
CGFloat const kPlacePageWidth = 360;
CGFloat const kLeftOffset = 12;
CGFloat const kTopOffset = 36;
CGFloat const kBottomOffset = 36;
}  // namespace

@interface MWMiPadPlacePageLayoutImpl ()<UITableViewDelegate>

@property(nonatomic) CGRect availableArea;

@end

@implementation MWMiPadPlacePageLayoutImpl

@synthesize ownerView = _ownerView;
@synthesize placePageView = _placePageView;
@synthesize delegate = _delegate;
@synthesize actionBar = _actionBar;

- (instancetype)initOwnerView:(UIView *)ownerView
                placePageView:(MWMPPView *)placePageView
                     delegate:(id<MWMPlacePageLayoutDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    _ownerView = ownerView;
    _availableArea = ownerView.frame;
    self.placePageView = placePageView;
    placePageView.tableView.delegate = self;
    _delegate = delegate;
    [self addShadow];
  }
  return self;
}

- (void)addShadow
{
  CALayer * layer = self.placePageView.layer;
  layer.masksToBounds = NO;
  layer.shadowColor = UIColor.blackColor.CGColor;
  layer.shadowRadius = 4.;
  layer.shadowOpacity = 0.24f;
  layer.shadowOffset = {0, -2};
  layer.shouldRasterize = YES;
  layer.rasterizationScale = [[UIScreen mainScreen] scale];
}

- (void)onShow
{
  auto ppView = self.placePageView;
  auto actionBar = self.actionBar;
  ppView.tableView.scrollEnabled = NO;
  actionBar.alpha = 0;
  ppView.alpha = 0;
  ppView.origin = {- kPlacePageWidth, self.topBound};
  [self.ownerView addSubview:ppView];

  place_page_layout::animate(^{
    ppView.alpha = 1;
    actionBar.alpha = 1;
    ppView.minX = self.leftBound;
  });

  [self.delegate onExpanded];
}

- (void)onClose
{
  auto ppView = self.placePageView;
  place_page_layout::animate(
      ^{
        ppView.maxX = 0;
        ppView.alpha = 0;
      },
      ^{
        self.placePageView = nil;
        self.actionBar = nil;
        [self.delegate destroyLayout];
      });
}

- (void)updateAvailableArea:(CGRect)frame
{
  if (CGRectEqualToRect(self.availableArea, frame))
    return;
  self.availableArea = frame;
  [self updateContentLayout];
  place_page_layout::animate(^{
    self.placePageView.minX = self.leftBound;
  });
}

- (void)updateContentLayout
{
  auto ppView = self.placePageView;
  CGFloat const placePageHeight = ppView.tableView.contentSize.height;
  CGFloat const screenHeight = self.availableArea.size.height;

  ppView.height = [self actualPlacePageViewHeightWithPlacePageHeight:placePageHeight
                                                        screenHeight:screenHeight];

  place_page_layout::animate(^{
    ppView.minY = self.topBound;
  });
}

- (CGFloat)actualPlacePageViewHeightWithPlacePageHeight:(CGFloat)placePageHeight
                                           screenHeight:(CGFloat)screenHeight
{
  auto ppView = self.placePageView;
  BOOL const isPlacePageWithinScreen = [self isPlacePage:placePageHeight withinScreen:screenHeight];
  ppView.tableView.scrollEnabled = !isPlacePageWithinScreen;
  return isPlacePageWithinScreen ? placePageHeight + ppView.top.height
                                 : screenHeight - kBottomOffset - kTopOffset;
}

- (BOOL)isPlacePage:(CGFloat)placePageHeight withinScreen:(CGFloat)screenHeight
{
  auto const placePageFullHeight = placePageHeight;
  auto const availableSpace = screenHeight - kTopOffset - kBottomOffset;
  return availableSpace > placePageFullHeight;
}

#pragma mark - Pan

- (void)didPan:(UIPanGestureRecognizer *)pan
{
  MWMPPView * view = self.placePageView;
  auto superview = view.superview;

  CGFloat const leftOffset = self.leftBound;
  view.minX += [pan translationInView:superview].x;
  view.minX = MIN(view.minX, leftOffset);
  [pan setTranslation:CGPointZero inView:superview];

  CGFloat const alpha = MAX(0.0, view.maxX) / (view.width + leftOffset);
  view.alpha = alpha;
  UIGestureRecognizerState const state = pan.state;
  if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled)
  {
    CGFloat constexpr designAlpha = 0.8;
    if (alpha < designAlpha)
    {
      [self.delegate closePlacePage];
    }
    else
    {
      place_page_layout::animate(^{
        view.minX = leftOffset;
        view.alpha = 1;
      });
    }
  }
}

#pragma mark - Top and left bound

- (CGFloat)topBound { return self.availableArea.origin.y + kTopOffset; }
- (CGFloat)leftBound { return self.availableArea.origin.x + kLeftOffset; }
#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto cell = [tableView cellForRowAtIndexPath:indexPath];
  if ([cell isKindOfClass:[MWMAdBanner class]])
    [static_cast<MWMAdBanner *>(cell) highlightButton];
}

#pragma mark - Properties

- (void)setPlacePageView:(MWMPPView *)placePageView
{
  if (placePageView)
  {
    placePageView.width = kPlacePageWidth;
    placePageView.anchorImage.hidden = YES;
    auto pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(didPan:)];
    [placePageView addGestureRecognizer:pan];
  }
  else
  {
    [_placePageView removeFromSuperview];
  }
  _placePageView = placePageView;
}

- (void)setActionBar:(MWMPlacePageActionBar *)actionBar
{
  if (actionBar)
  {
    auto superview = self.placePageView;
    actionBar.origin = {0., superview.height - actionBar.height};
    [superview addSubview:actionBar];
  }
  else
  {
    [_actionBar removeFromSuperview];
  }
  _actionBar = actionBar;
}

@end
