#import "MWMSearchTextField.h"
#import "MWMSearch.h"
#import "UIImageView+Coloring.h"
#import "SwiftBridge.h"

namespace
{
NSTimeInterval constexpr kOnSearchCompletedDelay = 0.2;
}  // namespace

@interface MWMSearchTextField ()<MWMSearchObserver>

@property(nonatomic) BOOL isSearching;

@end

@implementation MWMSearchTextField

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    [self setStaticIcon];
    self.leftViewMode = UITextFieldViewModeAlways;
    [MWMSearch addObserver:self];
  }
  return self;
}

- (CGRect)leftViewRectForBounds:(CGRect)bounds
{
  CGRect rect = [super leftViewRectForBounds:bounds];
  rect.origin.x += 8.0;
  return rect;
}

#pragma mark - Draw

- (void)drawPlaceholderInRect:(CGRect)rect
{
  [[self placeholder] drawInRect:rect withAttributes:@{NSFontAttributeName: self.font,
                                                       NSForegroundColorAttributeName: self.tintColor}];
}

-(void)layoutSubviews
{
  [super layoutSubviews];
  for (UIView* view in self.subviews) {
    if ([view isKindOfClass:[UIButton class]]) {
      UIButton* button = (UIButton*) view;
      [button setImage:[UIImage imageNamed:@"ic_search_clear_14"] forState:UIControlStateNormal];
      button.tintColor = self.tintColor;
    }
  }
}

#pragma mark - Properties

- (void)setIsSearching:(BOOL)isSearching
{
  if (_isSearching == isSearching)
    return;
  _isSearching = isSearching;
  if (isSearching)
  {
    UIActivityIndicatorView * view = [[UIActivityIndicatorView alloc]
        initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    view.autoresizingMask =
        UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    [view startAnimating];
    view.bounds = self.leftView.bounds;
    view.styleName = @"SearchSearchTextFieldIcon";
    self.leftView = view;
  }
  else
  {
    [self setStaticIcon];
  }
}

- (void)setStaticIcon
{
  self.leftView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ic_search"]];
  [self.leftView setStyleAndApply:@"SearchSearchTextFieldIcon"];
}

- (void)stopSpinner { self.isSearching = NO; }
#pragma mark - MWMSearchObserver

- (void)onSearchStarted
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector(stopSpinner)
                                             object:nil];
  self.isSearching = YES;
}

- (void)onSearchCompleted
{
  SEL const selector = @selector(stopSpinner);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self performSelector:selector withObject:nil afterDelay:kOnSearchCompletedDelay];
}

@end
