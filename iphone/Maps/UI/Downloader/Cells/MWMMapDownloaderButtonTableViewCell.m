#import "MWMMapDownloaderButtonTableViewCell.h"

@implementation MWMMapDownloaderButtonTableViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self config];
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  [self config];
}

- (void)config
{
  self.separatorInset = UIEdgeInsetsZero;
  self.layoutMargins = UIEdgeInsetsZero;
}

@end
