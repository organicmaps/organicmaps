#import "MWMCommon.h"
#import "MWMSearchManager.h"
#import "MWMSearchTabButtonsView.h"

static CGFloat const kIconToLabelSpacing = 4.0;

@interface MWMSearchTabButtonsView ()

@property (nonatomic) IBOutlet UIView * rootView;

@property (weak, nonatomic) IBOutlet UIButton * icon;
@property (weak, nonatomic) IBOutlet UILabel * label;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * iconLeft;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * iconTop;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * labelLeft;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * labelTop;

@property (weak, nonatomic) IBOutlet id <MWMSearchTabButtonsViewProtocol> delegate;

@end

@implementation MWMSearchTabButtonsView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    _rootView =
        [NSBundle.mainBundle loadNibNamed:self.class.className owner:self options:nil].firstObject;
    [self addSubview:self.rootView];
    CALayer * sl = self.layer;
    sl.shouldRasterize = YES;
    sl.rasterizationScale = UIScreen.mainScreen.scale;
  }
  return self;
}

- (IBAction)buttonTap
{
  if (self.icon.selected)
    return;

  [self.delegate tabButtonPressed:self];
}

#pragma mark - Layout

- (void)layoutPortrait
{
  CGFloat const contentHeight = self.icon.height + kIconToLabelSpacing + self.label.height;
  CGFloat const topOffset = (self.height - contentHeight) / 2.0;

  self.iconTop.constant = nearbyint(topOffset);
  self.labelTop.constant = nearbyint(topOffset + self.icon.height + kIconToLabelSpacing);

  self.iconLeft.constant = nearbyint((self.width - self.icon.width) / 2.0);
  self.labelLeft.constant = nearbyint((self.width - self.label.width) / 2.0);
}

- (void)layoutLandscape
{
  CGFloat const contentWidth = self.icon.width + kIconToLabelSpacing + self.label.width;
  CGFloat const leftOffset = (self.width - contentWidth) / 2.0;
  self.iconLeft.constant = nearbyint(leftOffset);
  self.labelLeft.constant = nearbyint(leftOffset + self.icon.width + kIconToLabelSpacing);

  self.iconTop.constant = nearbyint((self.height - self.icon.height) / 2.0);
  self.labelTop.constant = nearbyint((self.height - self.label.height) / 2.0);
}

- (void)layoutSubviews
{
  self.rootView.frame = self.bounds;
  if (self.height < 60.0)
    [self layoutLandscape];
  else
    [self layoutPortrait];
  [super layoutSubviews];
}

#pragma mark - Properties

- (void)setSelected:(BOOL)selected
{
  _selected = self.icon.selected = selected;
  self.label.textColor = selected ? UIColor.linkBlue : UIColor.blackSecondaryText;
}

- (void)setIconImage:(UIImage *)iconImage
{
  _iconImage = iconImage;
  [self.icon setImage:iconImage forState:UIControlStateNormal];
}

- (void)setLocalizedText:(NSString *)localizedText
{
  _localizedText = self.label.text = L(localizedText);
}

@end
