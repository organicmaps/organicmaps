#import "MWMiPhonePlacePageLayoutImpl.h"
#import "MWMPPPreviewLayoutHelper.h"
#import "MWMPlacePageLayout.h"
#import "SwiftBridge.h"

namespace
{

CGFloat const kTopPlacePageStopValue = 0.7;
CGFloat const kExpandedPlacePageStopValue = 0.45;
CGFloat const kLuftDraggingOffset = 30;

// Minimal offset for collapse. If place page offset is below this value we should hide place page.
CGFloat const kMinOffset = 1;
}  // namespace

typedef NS_ENUM(NSUInteger, MWMOffsetState) {
  MWMOffsetStatePreview,
  MWMOffsetStatePreviewPlus,
  MWMOffsetStateOpen,
  MWMOffsetStateFullScreen
};

typedef NS_ENUM(NSUInteger, MWMScrollDirection) {
  MWMScrollDirectionUp,
  MWMScrollDirectionDown
};

@interface MWMiPhonePlacePageLayoutImpl ()<UIScrollViewDelegate, UITableViewDelegate,
                                           MWMPPPreviewLayoutHelperDelegate>

@property(nonatomic) MWMPPScrollView * scrollView;
@property(nonatomic) MWMScrollDirection direction;
@property(nonatomic) MWMOffsetState state;

@property(nonatomic) CGFloat lastContentOffset;
@property(weak, nonatomic) MWMPPPreviewLayoutHelper * previewLayoutHelper;

@property(nonatomic) CGRect availableArea;
@property(nonatomic) BOOL isOffsetAnimated;

@end

@implementation MWMiPhonePlacePageLayoutImpl

@synthesize ownerView = _ownerView;
@synthesize placePageView = _placePageView;
@synthesize delegate = _delegate;
@synthesize actionBar = _actionBar;

- (instancetype)initOwnerView:(UIView *)ownerView
                placePageView:(MWMPPView *)placePageView
                     delegate:(id<MWMPlacePageLayoutDelegate>)delegate {
  self = [super init];
  if (self) {
    CGSize size = ownerView.size;
    _ownerView = ownerView;
    _availableArea = ownerView.frame;
    _placePageView = placePageView;
    placePageView.tableView.delegate = self;
    _delegate = delegate;
    [self setScrollView:[[MWMPPScrollView alloc] initWithFrame:ownerView.frame
                                                  inactiveView:placePageView]];
    placePageView.frame = {{0, size.height}, size};
  }
  return self;
}

- (void)onShow {
  self.state = [self.delegate isPreviewPlus] ? MWMOffsetStatePreviewPlus : MWMOffsetStatePreview;
  auto scrollView = self.scrollView;
  
  [scrollView setContentOffset:{ 0., kMinOffset }];

  dispatch_async(dispatch_get_main_queue(), ^{
    place_page_layout::animate(^{
      [self.actionBar setVisible:YES];
      auto const targetOffset =
          self.state == MWMOffsetStatePreviewPlus ? self.previewPlusContentOffset : self.previewContentOffset;
      [self setAnimatedContentOffset:targetOffset];
    });
  });
}

- (void)onClose {
  self.actionBar = nil;
  place_page_layout::animate(^{
    [self setAnimatedContentOffset:0];
  },^{
    id<MWMPlacePageLayoutDelegate> delegate = self.delegate;
    // Workaround for preventing a situation when the scroll view destroyed before an animation finished.
    [delegate onPlacePageTopBoundChanged:0];
    self.scrollView = nil;
    [delegate destroyLayout];
  });
}

- (void)updateAvailableArea:(CGRect)frame {
  if (CGRectEqualToRect(self.availableArea, frame))
    return;
  self.availableArea = frame;

  UIScrollView * sv = self.scrollView;
  sv.delegate = nil;
  sv.frame = frame;
  sv.delegate = self;
  CGSize size = frame.size;
  self.placePageView.minY = size.height;
  id<MWMPlacePageLayoutDelegate> delegate = self.delegate;
  if (delegate == nil) { return; }
  [delegate onPlacePageTopBoundChanged:self.scrollView.contentOffset.y];
  CGFloat previewOffset = [delegate isPreviewPlus] ? self.previewPlusContentOffset : self.previewContentOffset;
  [self setAnimatedContentOffset:(self.state == MWMOffsetStateOpen || self.state == MWMOffsetStateFullScreen) ? self.topContentOffset : previewOffset];
}

- (void)updateContentLayout {
  auto const & size = self.availableArea.size;
  self.scrollView.contentSize = {size.width, size.height + self.placePageView.height};
}

- (void)setPreviewLayoutHelper:(MWMPPPreviewLayoutHelper *)previewLayoutHelper {
  previewLayoutHelper.delegate = self;
  _previewLayoutHelper = previewLayoutHelper;
}

#pragma mark - MWMPPPreviewLayoutHelperDelegate

- (void)heightWasChanged {
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.state == MWMOffsetStatePreview)
      [self setAnimatedContentOffset:self.previewContentOffset];
    if (self.state == MWMOffsetStatePreviewPlus)
      [self setAnimatedContentOffset:self.previewPlusContentOffset];
  });
}

#pragma mark - UIScrollViewDelegate

- (BOOL)isPortrait {
  CGSize size = self.ownerView.size;
  return size.height > size.width;
}

- (CGFloat)openContentOffset {
  CGSize size = self.ownerView.size;
  CGFloat offset = self.isPortrait ? MAX(size.width, size.height) : MIN(size.width, size.height);
  return offset * kTopPlacePageStopValue;
}

- (CGFloat)topContentOffset {
  CGFloat target = self.openContentOffset;
  CGFloat ppViewMaxY = self.placePageView.tableView.maxY;
  return MIN(target, ppViewMaxY);
}

- (CGFloat)previewPlusContentOffset {
  CGSize size = self.ownerView.size;
  CGFloat offset = self.isPortrait ? MAX(size.width, size.height) : MIN(size.width, size.height);
  CGFloat actionBarHeight = self.actionBar.height;
  offset -= actionBarHeight;
  CGFloat previewPlusOffset = offset * kExpandedPlacePageStopValue + actionBarHeight;
  CGFloat previewOffset = [self previewContentOffset];
  if (previewPlusOffset < previewOffset + kLuftDraggingOffset) {
    return previewOffset + kLuftDraggingOffset;
  }
  return previewPlusOffset;
}

- (CGFloat)previewContentOffset {
  return self.previewLayoutHelper.height + self.actionBar.height - self.placePageView.top.height;
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {
  dispatch_async(dispatch_get_main_queue(), ^{
    self.isOffsetAnimated = NO;
  });
}

- (void)scrollViewDidScroll:(MWMPPScrollView *)scrollView {
  if (self.isOffsetAnimated)
    return;
  auto ppView = self.placePageView;
  if ([scrollView isEqual:ppView.tableView])
    return;

  auto const & offsetY = scrollView.contentOffset.y;
  id<MWMPlacePageLayoutDelegate> delegate = self.delegate;
  if (offsetY <= 0) {
    [delegate onPlacePageTopBoundChanged:0];
    [delegate closePlacePage];
    return;
  }

  auto const bounded = ppView.height + kLuftDraggingOffset;
  if (offsetY > bounded) {
    [scrollView setContentOffset:{0, bounded}];
    [delegate onPlacePageTopBoundChanged:bounded];
  } else {
    [delegate onPlacePageTopBoundChanged:offsetY];
  }

  self.direction = self.lastContentOffset < offsetY ? MWMScrollDirectionUp : MWMScrollDirectionDown;
  self.lastContentOffset = offsetY;
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  CGFloat actualOffset = scrollView.contentOffset.y;
  CGFloat openOffset = self.openContentOffset;
  CGFloat targetOffset = (*targetContentOffset).y;
  CGFloat previewOffset = self.previewContentOffset;
  CGFloat previewPlusOffset = self.previewPlusContentOffset;
  BOOL isPreviewPlus = [self.delegate isPreviewPlus];
  
  if (actualOffset > previewPlusOffset && actualOffset < openOffset) {
    if (self.direction == MWMScrollDirectionUp) {
      self.state = MWMOffsetStateOpen;
      (*targetContentOffset).y = openOffset;
    } else {
      self.state = isPreviewPlus ? MWMOffsetStatePreviewPlus : MWMOffsetStatePreview;
      (*targetContentOffset).y = isPreviewPlus ? previewPlusOffset : previewOffset;
    }
  } else if (actualOffset > previewOffset && actualOffset < previewPlusOffset) {
    if (self.direction == MWMScrollDirectionUp) {
      self.state = isPreviewPlus ? MWMOffsetStatePreviewPlus : MWMOffsetStateOpen;
      (*targetContentOffset).y = isPreviewPlus ? previewPlusOffset : openOffset;
    } else {
      self.state = MWMOffsetStatePreview;
      (*targetContentOffset).y = previewOffset;
    }
  } else if (actualOffset > openOffset && targetOffset < openOffset) {
    self.state = MWMOffsetStateOpen;
    (*targetContentOffset).y = openOffset;
  } else if (actualOffset < previewOffset) {
    (*targetContentOffset).y = 0;
  } else {
    self.state = MWMOffsetStateFullScreen;
  }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView
                  willDecelerate:(BOOL)decelerate {
  if (decelerate)
    return;

  CGFloat actualOffset = scrollView.contentOffset.y;
  CGFloat openOffset = self.openContentOffset;
  CGFloat previewOffset = self.previewContentOffset;
  CGFloat previewPlusOffset = self.previewPlusContentOffset;
  BOOL isPreviewPlus = [self.delegate isPreviewPlus];

  if (actualOffset < previewOffset + kLuftDraggingOffset) {
    self.state = MWMOffsetStatePreview;
    place_page_layout::animate(^{
      [self setAnimatedContentOffset:previewOffset];
    });
  } else if (actualOffset < previewPlusOffset) {
    if (self.direction == MWMScrollDirectionUp) {
      self.state = isPreviewPlus ? MWMOffsetStatePreviewPlus : MWMOffsetStateOpen;
      place_page_layout::animate(^{
        [self setAnimatedContentOffset:isPreviewPlus ? previewPlusOffset : openOffset];
      });
    } else {
      self.state = MWMOffsetStatePreview;
      place_page_layout::animate(^{
        [self setAnimatedContentOffset:previewOffset];
      });
    }
  } else if (actualOffset < openOffset) {
    if (self.direction == MWMScrollDirectionUp) {
      self.state = MWMOffsetStateOpen;
      place_page_layout::animate(^{
        [self setAnimatedContentOffset:openOffset];
      });
    } else {
      self.state = isPreviewPlus ? MWMOffsetStatePreviewPlus : MWMOffsetStatePreview;
      place_page_layout::animate(^{
        [self setAnimatedContentOffset:isPreviewPlus ? previewPlusOffset : previewOffset];
      });
    }
  } else {
    self.state = MWMOffsetStateFullScreen;
  }
}

- (void)setState:(MWMOffsetState)state {
  if (_state != state) {
    NSNumber *value = [NSNumber numberWithUnsignedInteger:state];
    [self.delegate logStateChangeEventWithValue:value];
  }
  _state = state;
  BOOL isTop = (state == MWMOffsetStateOpen || self.state == MWMOffsetStateFullScreen);
  self.placePageView.anchorImage.transform = isTop ? CGAffineTransformMakeRotation(M_PI)
  : CGAffineTransformIdentity;
  [self.previewLayoutHelper layoutInOpenState:isTop];
  if (isTop)
    [self.delegate onExpanded];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  if (indexPath.section != 0)
    return;

  UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
  if ([cell isKindOfClass:[MWMAdBanner class]]) {
    return;
  }

  CGFloat offset = 0;
  if (self.state == MWMOffsetStateOpen || self.state == MWMOffsetStateFullScreen) {
    BOOL isPreviewPlus = [self.delegate isPreviewPlus];
    self.state = isPreviewPlus ? MWMOffsetStatePreviewPlus : MWMOffsetStatePreview;
    offset = isPreviewPlus ? self.previewPlusContentOffset : self.previewContentOffset;
  } else {
    self.state = MWMOffsetStateOpen;
    offset = self.topContentOffset;
  }

  place_page_layout::animate(^{
    [self setAnimatedContentOffset:offset];
  });
}

#pragma mark - Properties

- (void)setAnimatedContentOffset:(CGFloat)offset {
  self.isOffsetAnimated = YES;
  [self.scrollView setContentOffset:{0, offset} animated:YES];
}

- (void)setScrollView:(MWMPPScrollView *)scrollView {
  if (scrollView) {
    scrollView.delegate = self;
    [scrollView addSubview:self.placePageView];
    [self.ownerView addSubview:scrollView];
  } else {
    _scrollView.delegate = nil;
    [_scrollView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [_scrollView removeFromSuperview];
  }
  _scrollView = scrollView;
}

- (void)setActionBar:(MWMPlacePageActionBar *)actionBar {
  if (actionBar) {
    UIView *superview = self.ownerView;
    [superview addSubview:actionBar];
    NSLayoutXAxisAnchor * leadingAnchor = superview.leadingAnchor;
    NSLayoutXAxisAnchor * trailingAnchor = superview.trailingAnchor;
    if (@available(iOS 11.0, *)) {
      UILayoutGuide * safeAreaLayoutGuide = superview.safeAreaLayoutGuide;
      leadingAnchor = safeAreaLayoutGuide.leadingAnchor;
      trailingAnchor = safeAreaLayoutGuide.trailingAnchor;
    }
    [actionBar.leadingAnchor constraintEqualToAnchor:leadingAnchor].active = YES;
    [actionBar.trailingAnchor constraintEqualToAnchor:trailingAnchor].active = YES;
    [actionBar setVisible:NO];
  } else {
    [_actionBar removeFromSuperview];
  }
  _actionBar = actionBar;
}

@end

