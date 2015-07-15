
#import "SearchCell.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

@interface SearchCell ()

@property (nonatomic, readwrite) UILabel * titleLabel;

@end

@implementation SearchCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleDefault;
  [self addSubview:self.separatorView];
  UIView * selectedView = [[UIView alloc] initWithFrame:self.bounds];
  selectedView.backgroundColor = [UIColor pressBackground];
  self.selectedBackgroundView = selectedView;
  [self configTitleLabel];
  return self;
}

- (void)configTitleLabel
{
  self.titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  self.titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
  self.titleLabel.backgroundColor = [UIColor clearColor];
  self.titleLabel.font = [UIFont regular16];
}

- (void)setTitle:(NSString *)title selectedRanges:(NSArray *)selectedRanges
{
  NSDictionary * selectedTitleAttributes = [self selectedTitleAttributes];
  NSDictionary * unselectedTitleAttributes = [self unselectedTitleAttributes];
  if (!title || !selectedTitleAttributes || !unselectedTitleAttributes)
  {
    self.titleLabel.text = @"";
    return;
  }
  NSMutableAttributedString * attributedTitle = [[NSMutableAttributedString alloc] initWithString:title];
  [attributedTitle addAttributes:unselectedTitleAttributes range:NSMakeRange(0, [title length])];
  for (NSValue * range in selectedRanges)
    [attributedTitle addAttributes:selectedTitleAttributes range:[range rangeValue]];
  self.titleLabel.attributedText = attributedTitle;
}

- (NSDictionary *)selectedTitleAttributes
{
  return nil;
}

- (NSDictionary *)unselectedTitleAttributes
{
  return nil;
}

- (void)layoutSubviews
{
  self.separatorView.maxY = self.height;
  self.selectedBackgroundView.frame = self.bounds;
  self.backgroundView.frame = self.bounds;
}

- (UIView *)separatorView
{
  if (!_separatorView)
  {
    _separatorView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 1)];
    _separatorView.backgroundColor = [UIColor blackDividers];
    _separatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _separatorView;
}

@end
