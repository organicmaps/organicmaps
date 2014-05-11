
#import "SearchUniversalCell.h"
#import "UIKitCategories.h"

@interface SearchUniversalCell ()

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UILabel * subtitleLabel;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UIImageView * iconImageView;

@end

@implementation SearchUniversalCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.subtitleLabel];
  [self.contentView addSubview:self.iconImageView];
  [self.contentView addSubview:self.distanceLabel];

  return self;
}

#define DISTANCE_FONT [UIFont fontWithName:@"HelveticaNeue" size:10]
#define TITLE_FONT [UIFont fontWithName:@"HelveticaNeue" size:17]
#define SUBTITLE_FONT [UIFont fontWithName:@"HelveticaNeue" size:12.5]
#define TITLE_LEFT_SHIFT 44
#define TITLE_RIGHT_SHIFT 35
#define MAX_TITLE_HEIGHT 200

- (void)setTitle:(NSString *)title selectedRange:(NSRange)selectedRange
{
  if (!title)
    title = @"";
  NSMutableAttributedString * attributedTitle = [[NSMutableAttributedString alloc] initWithString:title];
  if (selectedRange.location == NSNotFound)
  {
    [attributedTitle addAttributes:[self unselectedTitleAttributes] range:NSMakeRange(0, [title length])];
  }
  else
  {
    NSRange unselectedRange1 = NSMakeRange(0, selectedRange.location);
    NSRange unselectedRange2 = NSMakeRange(selectedRange.location + selectedRange.length, [title length] - selectedRange.location - selectedRange.length);

    [attributedTitle addAttributes:[self selectedTitleAttributes] range:selectedRange];
    [attributedTitle addAttributes:[self unselectedTitleAttributes] range:unselectedRange1];
    [attributedTitle addAttributes:[self unselectedTitleAttributes] range:unselectedRange2];
  }
  self.titleLabel.attributedText = attributedTitle;
}

- (void)setSubtitle:(NSString *)subtitle selectedRange:(NSRange)selectedRange
{
  if (!subtitle)
    subtitle = @"";
  NSMutableAttributedString * attributedSubtitle = [[NSMutableAttributedString alloc] initWithString:subtitle];
  if (selectedRange.location == NSNotFound)
  {
    [attributedSubtitle addAttributes:[self unselectedSubtitleAttributes] range:NSMakeRange(0, [subtitle length])];
  }
  else
  {
    NSRange unselectedRange1 = NSMakeRange(0, selectedRange.location);
    NSRange unselectedRange2 = NSMakeRange(selectedRange.location + selectedRange.length, [subtitle length] - selectedRange.location - selectedRange.length);

    [attributedSubtitle addAttributes:[self selectedSubtitleAttributes] range:selectedRange];
    [attributedSubtitle addAttributes:[self unselectedSubtitleAttributes] range:unselectedRange1];
    [attributedSubtitle addAttributes:[self unselectedSubtitleAttributes] range:unselectedRange2];
  }
  self.subtitleLabel.attributedText = attributedSubtitle;
}

- (NSDictionary *)selectedSubtitleAttributes
{
  static NSDictionary * selectedAttributes;
  if (!selectedAttributes)
    selectedAttributes = @{NSForegroundColorAttributeName : [UIColor colorWithColorCode:@"16b68a"], NSFontAttributeName : SUBTITLE_FONT};
  return selectedAttributes;
}

- (NSDictionary *)unselectedSubtitleAttributes
{
  static NSDictionary * unselectedAttributes;
  if (!unselectedAttributes)
    unselectedAttributes = @{NSForegroundColorAttributeName : [UIColor whiteColor], NSFontAttributeName : SUBTITLE_FONT};
  return unselectedAttributes;
}

- (NSDictionary *)selectedTitleAttributes
{
  static NSDictionary * selectedAttributes;
  if (!selectedAttributes)
    selectedAttributes = @{NSForegroundColorAttributeName : [UIColor colorWithColorCode:@"16b68a"], NSFontAttributeName : TITLE_FONT};
  return selectedAttributes;
}

- (NSDictionary *)unselectedTitleAttributes
{
  static NSDictionary * unselectedAttributes;
  if (!unselectedAttributes)
    unselectedAttributes = @{NSForegroundColorAttributeName : [UIColor whiteColor], NSFontAttributeName : TITLE_FONT};
  return unselectedAttributes;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  [self.distanceLabel sizeToFit];
  self.distanceLabel.width += 4;
  self.distanceLabel.maxX = self.width - 25;
  self.distanceLabel.minY = 12;

  CGFloat distanceWidth = [self.distanceLabel.text sizeWithDrawSize:CGSizeMake(100, 20) font:DISTANCE_FONT].width;
  self.titleLabel.width = self.width - distanceWidth - TITLE_LEFT_SHIFT - TITLE_RIGHT_SHIFT;
  self.titleLabel.minX = TITLE_LEFT_SHIFT;
  [self.titleLabel sizeToFit];
  if ([self.subtitleLabel.text length])
    self.titleLabel.minY = 8;
  else
    self.titleLabel.midY = self.height / 2 - 1.5;
  self.subtitleLabel.origin = CGPointMake(self.titleLabel.minX, self.titleLabel.maxY - 2);
}

+ (CGFloat)cellHeightWithTitle:(NSString *)title subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width
{
  CGFloat distanceWidth = [distance sizeWithDrawSize:CGSizeMake(100, 20) font:DISTANCE_FONT].width;
  CGFloat titleHeight = [title sizeWithDrawSize:CGSizeMake(width - distanceWidth - TITLE_LEFT_SHIFT - TITLE_RIGHT_SHIFT, MAX_TITLE_HEIGHT) font:TITLE_FONT].height;
  return MAX(50, titleHeight + ([subtitle length] ? 27 : 15));
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 70, 16)];
    _distanceLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    _distanceLabel.backgroundColor = [UIColor colorWithColorCode:@"16b78a"];
    _distanceLabel.font = DISTANCE_FONT;
    _distanceLabel.textColor = [UIColor whiteColor];
    _distanceLabel.textAlignment = NSTextAlignmentCenter;
  }
  return _distanceLabel;
}

- (UILabel *)subtitleLabel
{
  if (!_subtitleLabel)
  {
    _subtitleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 190, 16)];
    _subtitleLabel.backgroundColor = [UIColor clearColor];
    _subtitleLabel.textColor = [UIColor whiteColor];
  }
  return _subtitleLabel;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, MAX_TITLE_HEIGHT)];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.numberOfLines = 0;
    _titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  }
  return _titleLabel;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(17, 16, 18, 18)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}

@end
