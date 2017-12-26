#import "MWMPPView.h"
#import "MWMCommon.h"
#import "MWMPlacePageActionBar.h"
#import "SwiftBridge.h"

namespace
{
// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueObserving/Articles/KVOBasics.html
void * kContext = &kContext;
NSString * const kTableViewContentSizeKeyPath = @"contentSize";
CGFloat const kTableViewTopInset = -36;

}  // namespace

#pragma mark - MWMPPScrollView

@implementation MWMPPScrollView

- (instancetype)initWithFrame:(CGRect)frame inactiveView:(UIView *)inactiveView
{
  self = [super initWithFrame:frame];
  if (self)
  {
    self.decelerationRate = UIScrollViewDecelerationRateFast;
    self.showsVerticalScrollIndicator = NO;
    _inactiveView = inactiveView;
  }
  return self;
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  UIView * v = self.inactiveView;
  return point.y > [self convertRect:v.bounds fromView:v].origin.y;
}

@end

#pragma mark - MWMPPView

@implementation MWMPPView

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat const height = s.CGSizeValue.height;
    if (!equalScreenDimensions(height, self.currentContentHeight) && !self.tableView.scrollEnabled)
    {
      self.currentContentHeight = height;
      self.height = height + self.top.height;
      [self setNeedsLayout];
      [self.delegate updateLayout];
    }
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self.tableView addObserver:self
                   forKeyPath:kTableViewContentSizeKeyPath
                      options:NSKeyValueObservingOptionNew
                      context:kContext];

  self.tableView.estimatedRowHeight = 44.;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  self.tableView.contentInset = {.top = kTableViewTopInset, .bottom = - kTableViewTopInset};
  if (IPAD)
    self.autoresizingMask = UIViewAutoresizingNone;
}

- (void)dealloc
{
  [self.tableView removeObserver:self forKeyPath:kTableViewContentSizeKeyPath context:kContext];
}

#pragma mark - VisibleArea

- (MWMAvailableAreaAffectDirections)visibleAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsLeft : MWMAvailableAreaAffectDirectionsNone;
}

#pragma mark - AvailableArea / SideButtonsArea

- (MWMAvailableAreaAffectDirections)sideButtonsAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsNone : MWMAvailableAreaAffectDirectionsBottom;
}

@end
