#import "MWMTableViewCell.h"

@implementation MWMTableViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  return self;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
}

- (void)addSubview:(UIView *)view
{
  [super addSubview:view];
  if (self.isSeparatorHidden)
    [self hideSeparators];
}

- (void)setIsSeparatorHidden:(BOOL)isSeparatorHidden
{
  _isSeparatorHidden = isSeparatorHidden;
  isSeparatorHidden ? [self hideSeparators] : [self showSeparators];
}

- (void)hideSeparators
{
  for (UIView * view in self.subviews)
    view.hidden = [[[view class] className] isEqualToString:@"_UITableViewCellSeparatorView"];
}

- (void)showSeparators
{
  for (UIView * view in self.subviews)
    view.hidden = NO;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (self.isSeparatorHidden)
    [self hideSeparators];
}

@end

@implementation MWMTableViewSubtitleCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  return [super initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:reuseIdentifier];
}

@end
