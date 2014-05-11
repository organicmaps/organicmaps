
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
  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleNone;

  [self.contentView addSubview:self.shareButton];
  [self.contentView addSubview:self.apiButton];

  return self;
}

- (void)setApiAppTitle:(NSString *)appTitle
{
  CGFloat const xOffset = 20;
  CGFloat const yOffset = 14;
  CGFloat const height = [self.shareButton backgroundImageForState:UIControlStateNormal].size.height;
  if (appTitle)
  {
    CGFloat const betweenOffset = 10;
    self.shareButton.frame = CGRectMake(xOffset, yOffset, (self.width - 2 * xOffset - betweenOffset) / 2, height);
    self.apiButton.frame = CGRectMake(self.shareButton.maxX + betweenOffset, yOffset, (self.width - 2 * xOffset - betweenOffset) / 2, height);
    self.apiButton.hidden = NO;
    [self.apiButton setTitle:appTitle forState:UIControlStateNormal];
  }
  else
  {
    self.shareButton.frame = CGRectMake(xOffset, yOffset, self.width - 2 * xOffset, height);
    self.apiButton.hidden = YES;
  }
}

+ (CGFloat)cellHeight
{
  return 70;
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
    _shareButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _shareButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_shareButton setBackgroundImage:image forState:UIControlStateNormal];
    [_shareButton addTarget:self action:@selector(shareButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_shareButton setTitle:NSLocalizedString(@"share", nil) forState:UIControlStateNormal];
    [_shareButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
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
    _apiButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _apiButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_apiButton setBackgroundImage:image forState:UIControlStateNormal];
    [_apiButton addTarget:self action:@selector(apiButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_apiButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_apiButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  }
  return _apiButton;
}

@end
