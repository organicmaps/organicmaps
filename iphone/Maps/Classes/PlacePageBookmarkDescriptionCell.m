
#import "PlacePageBookmarkDescriptionCell.h"
#import "UIKitCategories.h"

@implementation PlacePageBookmarkDescriptionCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  UIView * selectedBackgroundView = [[UIView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.2];
  self.selectedBackgroundView = selectedBackgroundView;

  UIImageView * editImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"PlacePageEditButton"]];
  editImageView.center = CGPointMake(self.width - 24, 20);
  editImageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
  [self addSubview:editImageView];

  UIImage * separatorImage = [[UIImage imageNamed:@"PlacePageSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
  CGFloat const offset = 12.5;
  UIImageView * separator = [[UIImageView alloc] initWithFrame:CGRectMake(offset, self.height - separatorImage.size.height, self.width - 2 * offset, separatorImage.size.height)];
  separator.image = separatorImage;
  separator.maxY = self.height;
  separator.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  [self addSubview:separator];

  [self addSubview:self.titleLabel];

  return self;
}

#define LEFT_SHIFT 14
#define RIGHT_SHIFT 41
#define TITLE_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5]

- (void)layoutSubviews
{
  self.titleLabel.width = self.width - LEFT_SHIFT - RIGHT_SHIFT;
  [self.titleLabel sizeToIntegralFit];
  self.titleLabel.origin = CGPointMake(LEFT_SHIFT, 10);

  self.selectedBackgroundView.frame = self.bounds;
  self.backgroundColor = [UIColor clearColor];
}

- (void)setWebView:(UIWebView *)webView
{
  if (!_webView && !webView.isLoading)
  {
    [self.contentView addSubview:webView];
    webView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    CGFloat const xOffsetLeft = 6;
    CGFloat const xOffsetRight = 30;
    CGFloat const yOffset = 1;
    webView.frame = CGRectMake(xOffsetLeft, yOffset, self.width - xOffsetLeft - xOffsetRight, self.height - 2 * yOffset);
    webView.clipsToBounds = YES;
    webView.opaque = NO;
    webView.backgroundColor = [UIColor clearColor];
    webView.userInteractionEnabled = NO;
    _webView = webView;
  }
}

+ (CGFloat)cellHeightWithWebViewHeight:(CGFloat)webViewHeight
{
  return webViewHeight - 5;
}

+ (CGFloat)cellHeightWithTextValue:(NSString *)text viewWidth:(CGFloat)viewWidth
{
  CGFloat textHeight = [text sizeWithDrawSize:CGSizeMake(viewWidth - LEFT_SHIFT - RIGHT_SHIFT, 10000) font:TITLE_FONT].height;
  return textHeight + 24;
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
    _titleLabel.textColor = [UIColor blackColor];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _titleLabel;
}

@end
