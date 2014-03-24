
#import "SearchUniversalCell.h"
#import "UIKitCategories.h"

@interface SearchUniversalCell ()

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UILabel * subtitleLabel;
@property (nonatomic) UILabel * distanceLabel;

@end

@implementation SearchUniversalCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.subtitleLabel];
  [self.contentView addSubview:self.distanceLabel];

  return self;
}

#define DISTANCE_FONT [UIFont fontWithName:@"HelveticaNeue" size:15]
#define TITLE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:15]
#define TITLE_WIDTH_REST 70
#define TITLE_HEIGHT 60

- (void)layoutSubviews
{
  [super layoutSubviews];

  self.distanceLabel.maxX = self.width - 45;
  self.distanceLabel.midY = self.height / 2 - 1;

  CGFloat distanceWidth = [self.distanceLabel.text sizeWithDrawSize:CGSizeMake(100, 20) font:DISTANCE_FONT].width;
  self.titleLabel.width = self.width - distanceWidth - TITLE_WIDTH_REST;
  self.titleLabel.minX = 16;
  [self.titleLabel sizeToFit];
  if ([self.subtitleLabel.text length])
    self.titleLabel.minY = 5;
  else
    self.titleLabel.midY = self.height / 2 - 1.5;
  self.subtitleLabel.origin = CGPointMake(self.titleLabel.minX, self.titleLabel.maxY - 1);
}

+ (CGFloat)cellHeightWithTitle:(NSString *)title subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width
{
  CGFloat distanceWidth = [distance sizeWithDrawSize:CGSizeMake(100, 20) font:DISTANCE_FONT].width;
  CGFloat titleHeight = [title sizeWithDrawSize:CGSizeMake(width - distanceWidth - TITLE_WIDTH_REST, TITLE_HEIGHT) font:TITLE_FONT].height;
  return MAX(44, titleHeight + ([subtitle length] ? 27 : 15));
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 70, 16)];
    _distanceLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = DISTANCE_FONT;
    _distanceLabel.textColor = [UIColor blackColor];
    _distanceLabel.textAlignment = NSTextAlignmentRight;
    _distanceLabel.alpha = 0.5;
  }
  return _distanceLabel;
}

- (UILabel *)subtitleLabel
{
  if (!_subtitleLabel)
  {
    _subtitleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 190, 16)];
    _subtitleLabel.backgroundColor = [UIColor clearColor];
    _subtitleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:10];
    _subtitleLabel.textColor = [UIColor colorWithColorCode:@"666666"];
  }
  return _subtitleLabel;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, TITLE_HEIGHT)];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = TITLE_FONT;
    _titleLabel.textColor = [UIColor blackColor];
    _titleLabel.numberOfLines = 0;
    _titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  }
  return _titleLabel;
}

@end
