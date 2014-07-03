
#import "SearchResultCell.h"
#import "UIKitCategories.h"

@interface SearchResultCell ()

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UILabel * typeLabel;
@property (nonatomic) UILabel * subtitleLabel;
@property (nonatomic) UILabel * distanceLabel;

@end

@implementation SearchResultCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.typeLabel];
  [self.contentView addSubview:self.subtitleLabel];
  [self.contentView addSubview:self.distanceLabel];

  UIView * selectedBackgroundView = [[UIView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithColorCode:@"15d081"];
  self.selectedBackgroundView = selectedBackgroundView;

  return self;
}

#define DISTANCE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:12.5]
#define TYPE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:12.5]
#define TITLE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:17]
#define SUBTITLE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:12.5]
#define LEFT_SHIFT 20
#define RIGHT_SHIFT 20
#define SPACE 4

- (void)setTitle:(NSString *)title selectedRanges:(NSArray *)selectedRanges
{
  if (!title)
    title = @"";
    
  NSMutableAttributedString * attributedTitle = [[NSMutableAttributedString alloc] initWithString:title];
  [attributedTitle addAttributes:[self unselectedTitleAttributes] range:NSMakeRange(0, [title length])];
  for (NSValue * range in selectedRanges)
    [attributedTitle addAttributes:[self selectedTitleAttributes] range:[range rangeValue]];

  self.titleLabel.attributedText = attributedTitle;
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

  self.typeLabel.size = [[self class] typeSizeWithType:self.typeLabel.text];
  self.typeLabel.maxX = self.width - RIGHT_SHIFT;
  self.typeLabel.minY = 9;

  self.distanceLabel.width = 70;
  [self.distanceLabel sizeToFit];
  self.distanceLabel.maxX = self.width - RIGHT_SHIFT;
  self.distanceLabel.maxY = self.height - 8.5;

  self.titleLabel.size = [[self class] titleSizeWithTitle:self.titleLabel.text viewWidth:self.width typeSize:self.typeLabel.size];
  self.titleLabel.minX = LEFT_SHIFT;
  self.titleLabel.minY = self.typeLabel.minY - 4;

  self.subtitleLabel.size = CGSizeMake(self.width - self.distanceLabel.width - LEFT_SHIFT - RIGHT_SHIFT - SPACE, 16);
  self.subtitleLabel.minX = LEFT_SHIFT;
  self.subtitleLabel.maxY = self.distanceLabel.maxY;

  CGFloat const offset = 12.5;
  self.separatorView.width = self.width - 2 * offset;
  self.separatorView.minX = offset;
}

+ (CGSize)typeSizeWithType:(NSString *)type
{
  return [type sizeWithDrawSize:CGSizeMake(150, 22) font:TYPE_FONT];
}

+ (CGSize)titleSizeWithTitle:(NSString *)title viewWidth:(CGFloat)width typeSize:(CGSize)typeSize
{
  CGSize const titleDrawSize = CGSizeMake(width - typeSize.width - LEFT_SHIFT - RIGHT_SHIFT - SPACE, 300);
  return [title sizeWithDrawSize:titleDrawSize font:TITLE_FONT];
}

+ (CGFloat)cellHeightWithTitle:(NSString *)title type:(NSString *)type subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width
{
  CGSize const typeSize = [self typeSizeWithType:type];
  return MAX(50, [self titleSizeWithTitle:title viewWidth:width typeSize:typeSize].height + ([subtitle length] ? 29 : 15));
}

- (UILabel *)typeLabel
{
  if (!_typeLabel)
  {
    _typeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _typeLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    _typeLabel.font = TYPE_FONT;
    _typeLabel.backgroundColor = [UIColor clearColor];
    _typeLabel.textColor = [UIColor colorWithColorCode:@"c9c9c9"];
    _typeLabel.textAlignment = NSTextAlignmentRight;
  }
  return _typeLabel;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    _distanceLabel.textColor = [UIColor colorWithColorCode:@"c9c9c9"];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = DISTANCE_FONT;
    _distanceLabel.textAlignment = NSTextAlignmentRight;
  }
  return _distanceLabel;
}

- (UILabel *)subtitleLabel
{
  if (!_subtitleLabel)
  {
    _subtitleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _subtitleLabel.backgroundColor = [UIColor clearColor];
    _subtitleLabel.textColor = [UIColor whiteColor];
    _subtitleLabel.font = SUBTITLE_FONT;
  }
  return _subtitleLabel;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.numberOfLines = 0;
    _titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  }
  return _titleLabel;
}

@end
