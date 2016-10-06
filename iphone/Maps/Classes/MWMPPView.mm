#import "Common.h"
#import "MWMPPView.h"
#import "UIColor+MapsMeColor.h"

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
    _inactiveView = inactiveView;
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

- (void)hideTableView:(BOOL)isHidden
{
  if (isHidden)
  {
    self.tableView.alpha = 0.;
    self.spinner.hidden = NO;
    [self.spinner startAnimating];
  }
  else
  {
    self.tableView.alpha = 1.;
    self.spinner.hidden = YES;
    [self.spinner stopAnimating];
  }
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat const height = s.CGSizeValue.height;
    if (!equalScreenDimensions(height, self.currentContentHeight))
    {
      self.currentContentHeight = height;
      self.height = height + self.top.height;
      [self setNeedsLayout];
      [self.delegate updateWithHeight:self.height];
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

  self.tableView.contentInset = {.top = kTableViewTopInset};

  NSUInteger constexpr animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  NSString * postfix = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] =
        [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@_%@", @(i + 1), postfix]];

  self.spinner.animationDuration = 0.8;
  self.spinner.animationImages = animationImages;
  [self.spinner startAnimating];
}

- (void)dealloc
{
  [self.tableView removeObserver:self forKeyPath:kTableViewContentSizeKeyPath context:kContext];
}

@end
