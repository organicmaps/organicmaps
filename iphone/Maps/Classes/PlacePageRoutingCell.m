
#import "PlacePageRoutingCell.h"
#import "UIKitCategories.h"

@interface PlacePageRoutingCell ()

@property (nonatomic) UIButton * cancelButton;
@property (nonatomic) UIButton * toButton;

@end

@implementation PlacePageRoutingCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.selectionStyle = UITableViewCellSelectionStyleNone;

  [self addSubview:self.toButton];
  [self addSubview:self.cancelButton];

  return self;
}

- (void)layoutSubviews
{
  CGFloat const xOffset = 20;
  CGFloat const yOffset = 14;
  CGFloat const height = [self.toButton backgroundImageForState:UIControlStateNormal].size.height;
  CGFloat const betweenOffset = 10;
  self.cancelButton.frame = CGRectMake(xOffset, yOffset, (self.width - 2 * xOffset - betweenOffset) / 2, height);
  self.toButton.frame = CGRectMake(self.cancelButton.maxX + betweenOffset, yOffset, (self.width - 2 * xOffset - betweenOffset) / 2, height);
  self.backgroundColor = [UIColor clearColor];
}

- (void)cancelButtonPressed:(id)sender
{
  [self.delegate cancelRouting:self];
}

- (void)toButtonPressed:(id)sender
{
  [self.delegate routeCellDidSetEndPoint:self];
}

+ (CGFloat)cellHeight
{
  return 70;
}

- (UIButton *)cancelButton
{
  if (!_cancelButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _cancelButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _cancelButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _cancelButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_cancelButton setBackgroundImage:image forState:UIControlStateNormal];
    [_cancelButton addTarget:self action:@selector(cancelButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_cancelButton setTitle:NSLocalizedString(@"choose_starting_point", nil) forState:UIControlStateNormal];
    [_cancelButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_cancelButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  }
  return _cancelButton;
}

- (UIButton *)toButton
{
  if (!_toButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _toButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _toButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _toButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin;
    [_toButton setBackgroundImage:image forState:UIControlStateNormal];
    [_toButton addTarget:self action:@selector(toButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_toButton setTitle:NSLocalizedString(@"choose_destination", nil) forState:UIControlStateNormal];
    [_toButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_toButton setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  }
  return _toButton;
}

@end
