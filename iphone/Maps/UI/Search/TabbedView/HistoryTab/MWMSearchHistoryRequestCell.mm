#import "MWMCommon.h"
#import "MWMSearchHistoryRequestCell.h"

@interface MWMSearchHistoryRequestCell ()

@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryRequestCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)config:(NSString *)title
{
  UILabel * label = self.label;
  label.text = title;
  label.textColor = [UIColor blackSecondaryText];
}

@end
