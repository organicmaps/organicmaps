#import "MWMMapDownloaderSubplaceTableViewCell.h"

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * subPlace;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.subPlace.preferredMaxLayoutWidth = self.subPlace.width;
  [super layoutSubviews];
}

- (void)setSubplaceText:(NSString *)text
{
  self.subPlace.text = text;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 82.0;
}

@end
