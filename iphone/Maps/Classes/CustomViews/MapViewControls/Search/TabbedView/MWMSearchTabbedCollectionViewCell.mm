#import "MWMSearchTabbedCollectionViewCell.h"
#import "UIKitCategories.h"

@interface MWMSearchTabbedCollectionViewCell ()

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;

@end

@implementation MWMSearchTabbedCollectionViewCell

- (void)awakeFromNib
{
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)layoutSubviews
{
  CGFloat const textBottom = self.noResultsImage.height + self.noResultsTitle.height + self.noResultsText.height + 68.0;
  BOOL const compact = textBottom > self.height;
  self.titleTopOffset.constant = compact ? 20. : 196.;
  self.noResultsImage.hidden = compact;
  [super layoutSubviews];
}

@end
