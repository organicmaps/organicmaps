#import "MWMSearchTableView.h"
#import "UIKitCategories.h"

@interface MWMSearchTableView ()

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * textTopOffset;

@end

@implementation MWMSearchTableView

- (void)awakeFromNib
{
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)layoutSubviews
{
  CGFloat const textBottom = self.noResultsImage.height + self.noResultsText.height + 68.0;
  BOOL const compact = textBottom > self.height;
  self.textTopOffset.constant = compact ? 20. : 160.;
  self.noResultsImage.hidden = compact;
  [super layoutSubviews];
}

@end
