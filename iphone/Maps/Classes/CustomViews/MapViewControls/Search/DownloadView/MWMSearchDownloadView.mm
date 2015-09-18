#import "Common.h"
#import "MWMSearchDownloadView.h"

@interface MWMSearchDownloadView ()

@property (weak, nonatomic) IBOutlet UILabel * hint;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * hintTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * downloadRequestWrapperTopOffset;

@end

@implementation MWMSearchDownloadView

- (void)awakeFromNib
{
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)layoutSubviews
{
  self.frame = self.superview.bounds;
  [self update];
  [super layoutSubviews];
}

- (void)update
{
  UIView * superview = self.superview;
  if (!superview)
    return;
  BOOL const isPortrait = superview.width < superview.height;
  BOOL const isCompact = superview.height <= 480.0;
  BOOL const defaultSize = IPAD || (isPortrait && !isCompact);
  [self layoutIfNeeded];
  self.hintTopOffset.constant = defaultSize ? 40.0 : 12.0;
  if (self.state == MWMSearchDownloadViewStateProgress)
    self.downloadRequestWrapperTopOffset.constant = isPortrait ? 100.0 : 40.0;
  else
    self.downloadRequestWrapperTopOffset.constant = defaultSize ? 200.0 : 12.0;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    if (self.state == MWMSearchDownloadViewStateProgress)
    {
      self.hint.alpha = self.image.alpha = 0.0;
    }
    else
    {
      self.hint.alpha = 1.0;
      self.image.alpha = defaultSize ? 1.0 : 0.0;
    }
    [self layoutIfNeeded];
  }];
}

#pragma mark - Property

- (void)setState:(enum MWMSearchDownloadViewState)state
{
  if (_state == state)
    return;
  _state = state;
  [self update];
}

@end
