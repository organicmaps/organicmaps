
#import "PlacePageEditCell.h"
#import "UIKitCategories.h"

@interface PlacePageEditCell ()

@end

@implementation PlacePageEditCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];

  UIView * selectedBackgroundView = [[UIView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.2];
  self.selectedBackgroundView = selectedBackgroundView;

  UIImageView * editImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"PlacePageEditButton"]];
  editImageView.center = CGPointMake(self.width - 30, 20);
  editImageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
  [self addSubview:editImageView];

  UIImage * separatorImage = [[UIImage imageNamed:@"PlacePageSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
  CGFloat const offset = 15;
  UIImageView * separator = [[UIImageView alloc] initWithFrame:CGRectMake(offset, self.height - separatorImage.size.height, self.width - 2 * offset, separatorImage.size.height)];
  separator.image = separatorImage;
  separator.maxY = self.height;
  separator.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  [self addSubview:separator];

  return self;
}

#define LEFT_SHIFT 19
#define RIGHT_SHIFT 47
#define TITLE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5]

+ (CGFloat)cellHeightWithTextValue:(NSString *)text viewWidth:(CGFloat)viewWidth
{
  CGFloat textHeight = [text sizeWithDrawSize:CGSizeMake(viewWidth - LEFT_SHIFT - RIGHT_SHIFT, 10000) font:TITLE_FONT].height;
  return textHeight + 24;
}

- (void)layoutSubviews
{
  self.titleLabel.width = self.width - LEFT_SHIFT - RIGHT_SHIFT;
  [self.titleLabel sizeToFit];
  self.titleLabel.origin = CGPointMake(LEFT_SHIFT, 10);

  self.selectedBackgroundView.frame = self.bounds;
  self.backgroundColor = [UIColor clearColor];
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = TITLE_FONT;
    _titleLabel.textAlignment = NSTextAlignmentLeft;
    _titleLabel.numberOfLines = 0;
    _titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    _titleLabel.textColor = [UIColor whiteColor];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _titleLabel;
}

@end
