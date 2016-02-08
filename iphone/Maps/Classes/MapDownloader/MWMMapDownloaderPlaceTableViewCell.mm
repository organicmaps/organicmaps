#import "MWMMapDownloaderPlaceTableViewCell.h"

@interface MWMMapDownloaderPlaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * area;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleBottomOffset;

@end

@implementation MWMMapDownloaderPlaceTableViewCell

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.area.preferredMaxLayoutWidth = self.area.width;
  [super layoutSubviews];
}

- (void)setAreaText:(NSString *)text
{
  self.area.text = text;
  BOOL const isAreaHidden = (text.length == 0);
  self.area.hidden = isAreaHidden;
  self.titleBottomOffset.priority = isAreaHidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
