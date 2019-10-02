#import "MWMMapDownloaderButtonTableViewCell.h"

//#include "storage/storage.hpp"
//
//using namespace storage;

@implementation MWMMapDownloaderButtonTableViewCell

+ (CGFloat)estimatedHeight
{
  return 44.0;
}

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

- (IBAction)buttonPressed
{
  [self.delegate openAvailableMaps];
}

@end
