#import "MWMPlacePageLayout.h"
#import "MWMiPhonePlacePageLayoutImpl.h"
#import "MWMPPPreviewLayoutHelper.h"

namespace
{
enum class ScrollDirection
{
  Up,
  Down
};

enum class State
{
  Bottom,
  Top
};

CGFloat const kOpenPlacePageStopValue = 0.7;
CGFloat const kLuftDraggingOffset = 30;

// Minimal offset for collapse. If place page offset is below this value we should hide place page.
CGFloat const kMinOffset = 1;
}  // namespace

@interface MWMiPhonePlacePageLayoutImpl () <UIScrollViewDelegate, UITableViewDelegate>

@property(nonatomic) MWMPPScrollView * scrollView;
@property(nonatomic) ScrollDirection direction;
@property(nonatomic) State state;

@property(nonatomic) CGFloat portraitOpenContentOffset;
@property(nonatomic) CGFloat landscapeOpenContentOffset;
@property(nonatomic) CGFloat lastContentOffset;
@property(nonatomic) CGFloat expandedContentOffset;
@property(weak, nonatomic) MWMPPPreviewLayoutHelper * previewLayoutHelper;

@end

@implementation MWMiPhonePlacePageLayoutImpl

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
    auto const & size = ownerView.size;
    _ownerView = ownerView;
    _placePageView = placePageView;
    placePageView.tableView.delegate = self;
    _delegate = delegate;
    self.scrollView =
        [[MWMPPScrollView alloc] initWithFrame:ownerView.frame inactiveView:placePageView];
    _portraitOpenContentOffset = MAX(size.width, size.height) * kOpenPlacePageStopValue;
    _landscapeOpenContentOffset = MIN(size.width, size.height) * kOpenPlacePageStopValue;
    placePageView.frame = {{0, size.height}, size};
  }
  return self;
}

- (void)onShow
{
  self.state = State::Bottom;
  auto scrollView = self.scrollView;
  
  scrollView.scrollEnabled = NO;
  [scrollView setContentOffset:{ 0., kMinOffset }];

  dispatch_async(dispatch_get_main_queue(), ^{
    place_page_layout::animate(^{
      scrollView.scrollEnabled = YES;
      auto actionBar = self.actionBar;
      actionBar.maxY = actionBar.superview.height;
      self.expandedContentOffset =
        self.previewLayoutHelper.height + actionBar.height - self.placePageView.top.height;
      auto const targetOffset =
        self.state == State::Bottom ? self.expandedContentOffset : self.topContentOffset;
      [scrollView setContentOffset:{ 0, targetOffset } animated:YES];
    });
  });
}

- (void)onClose
{
  place_page_layout::animate(^{
    self.actionBar.minY = self.ownerView.height;
    [self.scrollView setContentOffset:{} animated:YES];
  },^{
    self.actionBar = nil;
    self.scrollView = nil;
    [self.delegate shouldDestroyLayout];
  });
}

- (void)onScreenResize:(CGSize const &)size
{
  UIScrollView * sv = self.scrollView;
  sv.frame = {{}, size};
  self.placePageView.minY = size.height;
  auto actionBar = self.actionBar;
  actionBar.frame = {{0., size.height - actionBar.height},
    {size.width, actionBar.height}};
  [self.delegate onPlacePageTopBoundChanged:self.scrollView.contentOffset.y];
  [sv setContentOffset:{0, self.state == State::Top ? self.topContentOffset : self.expandedContentOffset}
                           animated:YES];
}

- (void)onUpdatePlacePageWithHeight:(CGFloat)height
{
  auto const & size = self.ownerView.size;
  self.scrollView.contentSize = {size.width, size.height + self.placePageView.height};
}

#pragma mark - UIScrollViewDelegate

- (BOOL)isPortrait
{
  auto const & s = self.ownerView.size;
  return s.height > s.width;
}

- (CGFloat)openContentOffset
{
  return self.isPortrait ? self.portraitOpenContentOffset : self.landscapeOpenContentOffset;
}

- (CGFloat)topContentOffset
{
  auto const target = self.openContentOffset;
  auto const ppView = self.placePageView;
  return MIN(target, ppView.height);
}

- (void)scrollViewDidScroll:(MWMPPScrollView *)scrollView
{
  auto ppView = self.placePageView;
  if ([scrollView isEqual:ppView.tableView])
    return;

  auto const & offset = scrollView.contentOffset;
  id<MWMPlacePageLayoutDelegate> delegate = self.delegate;
  if (offset.y <= 0)
  {
    [delegate onPlacePageTopBoundChanged:0];
    [delegate shouldClose];
    return;
  }

  if (offset.y > ppView.height + kLuftDraggingOffset)
  {
    auto const bounded = ppView.height + kLuftDraggingOffset;
    [scrollView setContentOffset:{0, bounded}];
    [delegate onPlacePageTopBoundChanged:bounded];
  }
  else
  {
    [delegate onPlacePageTopBoundChanged:offset.y];
  }

  self.direction = self.lastContentOffset < offset.y ? ScrollDirection::Up : ScrollDirection::Down;
  self.lastContentOffset = offset.y;
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset
{
  auto const actualOffset = scrollView.contentOffset.y;
  auto const openOffset = self.openContentOffset;
  auto const targetOffset = (*targetContentOffset).y;

  if (actualOffset > self.expandedContentOffset && actualOffset < openOffset)
  {
    auto const isDirectionUp = self.direction == ScrollDirection::Up;
    self.state = isDirectionUp ? State::Top : State::Bottom;
    (*targetContentOffset).y = isDirectionUp ? openOffset : self.expandedContentOffset;
  }
  else if (actualOffset > openOffset && targetOffset < openOffset)
  {
    self.state = State::Top;
    (*targetContentOffset).y = openOffset;
  }
  else if (actualOffset < self.expandedContentOffset)
  {
    (*targetContentOffset).y = 0;
    place_page_layout::animate(^{
      self.actionBar.minY = self.ownerView.height;
    });
  }
  else
  {
    self.state = State::Top;
  }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate
{
  if (decelerate)
    return;

  auto const actualOffset = scrollView.contentOffset.y;
  auto const openOffset = self.openContentOffset;

  if (actualOffset < self.expandedContentOffset + kLuftDraggingOffset)
  {
    self.state = State::Bottom;
    place_page_layout::animate(^{
      [scrollView setContentOffset:{ 0, self.expandedContentOffset } animated:YES];
    });
  }
  else if (actualOffset < openOffset)
  {
    auto const isDirectionUp = self.direction == ScrollDirection::Up;
    self.state = isDirectionUp ? State::Top : State::Bottom;
    place_page_layout::animate(^{
      [scrollView setContentOffset:{0, isDirectionUp ? openOffset : self.expandedContentOffset}
                          animated:YES];
    });
  }
  else
  {
    self.state = State::Top;
  }
}

- (void)setState:(State)state
{
  _state = state;
  BOOL const isTop = state == State::Top;
  self.placePageView.anchorImage.transform = isTop ? CGAffineTransformMakeRotation(M_PI)
  : CGAffineTransformIdentity;
  [self.previewLayoutHelper layoutInOpenState:isTop];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section != 0)
    return;

  CGFloat offset = 0;
  if (self.state == State::Top)
  {
    self.state = State::Bottom;
    offset = self.expandedContentOffset;
  }
  else
  {
    self.state = State::Top;
    offset = self.topContentOffset;
  }

  place_page_layout::animate(^{ [self.scrollView setContentOffset:{0, offset} animated:YES]; });
}

#pragma mark - Properties

- (void)setScrollView:(MWMPPScrollView *)scrollView
{
  if (scrollView)
  {
    scrollView.delegate = self;
    [scrollView addSubview:self.placePageView];
    [self.ownerView addSubview:scrollView];
  }
  else
  {
    _scrollView.delegate = nil;
    [_scrollView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [_scrollView removeFromSuperview];
  }
  _scrollView = scrollView;
}

- (void)setActionBar:(MWMPlacePageActionBar *)actionBar
{
  if (actionBar)
  {
    auto superview = self.ownerView;
    actionBar.minY = superview.height;
    [superview addSubview:actionBar];
  }
  else
  {
    [_actionBar removeFromSuperview];
  }
  _actionBar = actionBar;
}

@end

