#import "Common.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * subPlace;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

+ (CGFloat)estimatedHeight
{
  return 82.0;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.subPlace.preferredMaxLayoutWidth = floor(self.subPlace.width);
    [super layoutSubviews];
  }
}

- (void)setSubplaceText:(NSString *)text
{
  self.subPlace.text = text;
}

@end
