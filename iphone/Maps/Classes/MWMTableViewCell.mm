#import "MWMTableViewCell.h"

@implementation MWMTableViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self)
    [self configure];
  return self;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self configure];
}

- (void)configure
{
  self.backgroundColor = [UIColor white];
  self.textLabel.textColor = [UIColor blackPrimaryText];
  self.detailTextLabel.textColor = [UIColor blackSecondaryText];
  self.selectedBackgroundView = [[UIView alloc] init];
  self.selectedBackgroundView.backgroundColor = [UIColor pressBackground];
}

@end

@implementation MWMTableViewSubtitleCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  return [super initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:reuseIdentifier];
}

@end
