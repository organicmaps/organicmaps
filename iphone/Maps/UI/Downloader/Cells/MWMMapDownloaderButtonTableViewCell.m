#import "MWMMapDownloaderButtonTableViewCell.h"
#import "SwiftBridge.h"

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
  self.isAccessibilityElement = YES;
  self.accessibilityLabel = L(@"download_maps");
  self.accessibilityTraits = UIAccessibilityTraitButton;
}

@end
