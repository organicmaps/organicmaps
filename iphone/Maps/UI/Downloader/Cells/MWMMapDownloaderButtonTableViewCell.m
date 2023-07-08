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
  if ([self respondsToSelector:@selector(setSeparatorInset:)])
    [self setSeparatorInset:UIEdgeInsetsZero];
  if ([self respondsToSelector:@selector(setLayoutMargins:)])
    [self setLayoutMargins:UIEdgeInsetsZero];
}

@end
