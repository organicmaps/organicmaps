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

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  BOOL const isAreaVisible = (self.needDisplayArea && nodeAttrs.m_parentInfo.size() == 1);
  if (isAreaVisible)
    self.area.text = @(nodeAttrs.m_parentInfo[0].m_localName.c_str());
  self.area.hidden = !isAreaVisible;
  self.titleBottomOffset.priority = isAreaVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
