#import "MWMDropDown.h"

static CGFloat const kLeadingOffset = 16.;
static CGFloat const kBottomOffset = 3.;
static CGFloat const kTopOffset = 25.;

#pragma mark - _MWMDropDownView (private)

@interface _MWMDropDownView : UIView

@property (weak, nonatomic) IBOutlet UILabel * message;

@end

@implementation _MWMDropDownView

- (void)layoutSubviews
{
  CGFloat superviewWidth = self.superview.width;
  self.message.width = superviewWidth - 2 * kLeadingOffset;
  [self.message sizeToFit];
  self.size = CGSizeMake(superviewWidth, kTopOffset + kBottomOffset + self.message.height);
  self.message.midX = self.superview.midX;
  [super layoutSubviews];
}

@end

#pragma mark - MWMDropDown implementation

@interface MWMDropDown ()

@property (nonatomic) IBOutlet _MWMDropDownView * dropDown;
@property (weak, nonatomic) UIView * superview;

@end

@implementation MWMDropDown

#pragma mark - Public

- (instancetype)initWithSuperview:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:[[self class] className] owner:self options:nil];
    _superview = view;
  }
  return self;
}

- (void)showWithMessage:(NSString *)message
{
  [self.dropDown removeFromSuperview];
  self.dropDown.message.text = message;
  self.dropDown.alpha = 0.;
  [self.dropDown setNeedsLayout];
  [self.superview addSubview:self.dropDown];
  self.dropDown.origin = CGPointMake(0., -self.dropDown.height);
  [self show];
}

- (void)dismiss
{
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.dropDown.alpha = 0.;
    self.dropDown.origin = CGPointMake(0., -self.dropDown.height);
  }
  completion:^(BOOL finished)
  {
    [self.dropDown removeFromSuperview];
  }];
}

#pragma mark - Private

- (void)show
{
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.dropDown.alpha = 1.;
    self.dropDown.origin = CGPointZero;
  }];

  [self performAfterDelay:3 block:^
  {
    [self dismiss];
  }];
}

@end
