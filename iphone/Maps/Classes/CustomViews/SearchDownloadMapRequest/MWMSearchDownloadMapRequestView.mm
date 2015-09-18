#import "Common.h"
#import "MWMSearchDownloadMapRequest.h"
#import "MWMSearchDownloadMapRequestView.h"

@interface MWMSearchDownloadMapRequestView ()

@property (weak, nonatomic) IBOutlet UILabel * hint;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * hintTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * downloadRequestWrapperBottomOffset;
@property (weak, nonatomic) IBOutlet UIView * downloadRequestWrapper;
@property (weak, nonatomic) IBOutlet MWMSearchDownloadMapRequest * owner;

@property (nonatomic) enum MWMSearchDownloadMapRequestViewState state;

@end

@implementation MWMSearchDownloadMapRequestView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return self;
}

- (void)show:(enum MWMSearchDownloadMapRequestViewState)state
{
  [self layoutIfNeeded];
  self.state = state;
  [self update];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.frame = superview.bounds;
  [self update];
  [super layoutSubviews];
}

- (void)update
{
  UIView * superview = self.superview;
  BOOL const isPortrait = superview.width < superview.height;
  if (self.state == MWMSearchDownloadMapRequestViewStateProgress)
  {
    self.hint.alpha = 0.0;
    CGFloat const offset = 0.5 * self.height - self.downloadRequestWrapper.height;
    self.downloadRequestWrapperBottomOffset.constant = offset;
  }
  else
  {
    self.hint.alpha = 1.0;
    self.downloadRequestWrapperBottomOffset.constant = 0.0;
  }
  CGFloat const topOffset = (IPAD || isPortrait ? 40.0 : 12.0);
  // @TODO Remove on new search!
  CGFloat const someCrazyTopOfsetForCurrentSearchViewImplementation = 64.0;
  self.hintTopOffset.constant = someCrazyTopOfsetForCurrentSearchViewImplementation + topOffset;
}

@end
