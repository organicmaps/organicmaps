
#import "PlacePageShareCell.h"
#import "UIKitCategories.h"

@interface PlacePageShareCell ()

@property (nonatomic) UIButton * shareButton;
@property (nonatomic) UIButton * apiButton;

@end

@implementation PlacePageShareCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  self.selectionStyle = UITableViewCellSelectionStyleNone;

  [self addSubview:self.shareButton];
  [self addSubview:self.apiButton];

  return self;
}

- (void)layoutSubviews
{
  CGFloat const xOffset = 20;
  CGFloat const height = [self.shareButton backgroundImageForState:UIControlStateNormal].size.height;
  if (self.apiAppTitle)
  {
    CGFloat const betweenOffset = 10;
    self.shareButton.frame = CGRectMake(xOffset, 0, (self.width - 2 * xOffset - betweenOffset) / 2, height);
    self.apiButton.frame = CGRectMake(self.shareButton.maxX + betweenOffset, 0, (self.width - 2 * xOffset - betweenOffset) / 2, height);
    self.apiButton.hidden = NO;
    [self.apiButton setTitle:self.apiAppTitle forState:UIControlStateNormal];
  }
  else
  {
    self.shareButton.frame = CGRectMake(xOffset, 0, self.width - 2 * xOffset, height);
    self.apiButton.hidden = YES;
  }
  self.apiButton.midY = self.height / 2;
  self.shareButton.midY = self.height / 2;

  self.backgroundColor = [UIColor clearColor];
}

+ (CGFloat)cellHeight
{
  return 54;
}

- (void)shareButtonPressed:(id)sender
{
  [self.delegate shareCellDidPressShareButton:self];
}

- (void)apiButtonPressed:(id)sender
{
  [self.delegate shareCellDidPressApiButton:self];
}

- (UIButton *)shareButton
{
  if (!_shareButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _shareButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _shareButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _shareButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_shareButton setBackgroundImage:image forState:UIControlStateNormal];
    [_shareButton addTarget:self action:@selector(shareButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_shareButton setTitle:NSLocalizedString(@"share", nil) forState:UIControlStateNormal];
    [_shareButton setTitleColor:[UIColor colorWithColorCode:@"179E4D"] forState:UIControlStateNormal];
    [_shareButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  }
  return _shareButton;
}

- (UIButton *)apiButton
{
  if (!_apiButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _apiButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _apiButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _apiButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_apiButton setBackgroundImage:image forState:UIControlStateNormal];
    [_apiButton addTarget:self action:@selector(apiButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_apiButton setTitleColor:[UIColor colorWithColorCode:@"179E4D"] forState:UIControlStateNormal];
    [_apiButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  }
  return _apiButton;
}

@end
