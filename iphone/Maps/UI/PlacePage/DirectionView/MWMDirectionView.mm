#import "MWMDirectionView.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

static NSString * const kDirectionViewNibName = @"MWMDirectionView";
static CGFloat const kDirectionArrowSide = IPAD ? 260. : 160.;

@interface MWMDirectionView ()

@property (nonatomic) CGSize defaultSize;

@end

@implementation MWMDirectionView

- (instancetype)init
{
  self = [NSBundle.mainBundle loadNibNamed:kDirectionViewNibName owner:nil options:nil].firstObject;
  [self setup];
  return self;
}

- (void)setup
{
  self.directionArrow.size = CGSizeMake(kDirectionArrowSide, kDirectionArrowSide);
  self.directionArrow.image = [UIImage imageNamed:IPAD ? @"direction_big" : @"direction_mini"];

  self.distanceLabel.font = IPAD ? [UIFont regular52] : [UIFont regular32];
  self.typeLabel.font = IPAD ? [UIFont regular24] : [UIFont regular16];

  self.autoresizingMask = self.contentView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;
}

- (void)show
{
  UIView * superview = [MapsAppDelegate theApp].mapViewController.view;
  [superview addSubview:self];
  [superview endEditing:YES];
  [self setNeedsLayout];
}

- (void)didMoveToSuperview
{
  [super didMoveToSuperview];

  MapsAppDelegate * app = [MapsAppDelegate theApp];

  MWMMapViewControlsManager * manager = [MWMMapViewControlsManager manager];

  if (self.superview)
  {
    [app disableStandby];
    manager.isDirectionViewHidden = NO;
  }
  else
  {
    [app enableStandby];
    manager.isDirectionViewHidden = YES;
  }

  [app.mapViewController updateStatusBarStyle];
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.frame = superview.bounds;
  CGSize const size = self.superview.size;
  self.size = size;
  CGFloat const minimumBorderOffset = 40.;
  BOOL const isLandscape = size.width > size.height;
  if (isLandscape)
  {
    CGFloat const defaultWidth = size.width - 2. * minimumBorderOffset - kDirectionArrowSide;
    [self resizeTitleWithWidth:defaultWidth];
    [self resizeTypeAndDistanceWithWidth:defaultWidth];
    CGFloat const titleOffset = 8.;
    CGFloat const typeOffset = 24.;
    CGFloat const arrowOffset = 24.;
    CGFloat const contentViewHeight = size.height - 2. * minimumBorderOffset;
    CGFloat const contentViewOffset = (size.width - self.titleLabel.width - minimumBorderOffset - self.directionArrow.width) / 2.;
    CGFloat const contentViewWidth = self.titleLabel.width + minimumBorderOffset + self.directionArrow.width;
    self.contentView.frame = CGRectMake(contentViewOffset, minimumBorderOffset, contentViewWidth, contentViewHeight);
    self.directionArrow.center = CGPointMake(arrowOffset + kDirectionArrowSide / 2., self.contentView.height / 2.);
    CGFloat const directionArrowOffsetX = self.directionArrow.maxX + minimumBorderOffset;
    CGFloat const actualLabelsBlockHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + typeOffset + self.distanceLabel.height;
    CGFloat const labelsBlockTopOffset = (contentViewHeight - actualLabelsBlockHeight) / 2.;
    self.titleLabel.origin = CGPointMake(directionArrowOffsetX, labelsBlockTopOffset);
    self.titleLabel.textAlignment = NSTextAlignmentLeft;
    self.typeLabel.origin = CGPointMake(directionArrowOffsetX, self.titleLabel.maxY + titleOffset);
    self.typeLabel.textAlignment = NSTextAlignmentLeft;
    self.distanceLabel.origin = CGPointMake(directionArrowOffsetX, self.typeLabel.maxY + typeOffset);
    self.distanceLabel.textAlignment = NSTextAlignmentLeft;
  }
  else
  {
    CGFloat const defaultWidth = size.width - 2. * minimumBorderOffset;
    [self resizeTitleWithWidth:defaultWidth];
    [self resizeTypeAndDistanceWithWidth:defaultWidth];
    CGFloat const titleOffset = IPAD ? 12. : 8.;
    CGFloat const arrowOffset = IPAD ? 80. : 32.;
    CGFloat const contentViewActualHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + 2. * arrowOffset + kDirectionArrowSide + self.distanceLabel.height;
    CGFloat const contentViewSize = size.height > contentViewActualHeight ? contentViewActualHeight : size.height;
    CGFloat const yOffset = (size.height - contentViewSize) / 2.;
    self.contentView.frame = CGRectMake(minimumBorderOffset, yOffset, defaultWidth, contentViewSize);
    CGFloat const xOffset = self.contentView.width / 2.;
    self.titleLabel.origin = CGPointMake(xOffset - self.titleLabel.width / 2., 0.);
    self.titleLabel.textAlignment = NSTextAlignmentCenter;
    self.typeLabel.origin = CGPointMake(xOffset - self.typeLabel.width / 2., self.titleLabel.maxY + titleOffset);
    self.typeLabel.textAlignment = NSTextAlignmentCenter;
    self.directionArrow.center = CGPointMake(xOffset, self.typeLabel.maxY + arrowOffset + kDirectionArrowSide / 2.);
    self.distanceLabel.origin = CGPointMake(xOffset - self.distanceLabel.width / 2., self.directionArrow.maxY + arrowOffset);
    self.distanceLabel.textAlignment = NSTextAlignmentCenter;
  }
}

- (void)resizeTitleWithWidth:(CGFloat)width
{
  CGFloat const fontSize = IPAD ? 52.0 : 32.0;
  CGFloat const minFontSize = IPAD ? 24.0 : 16.0;
  CGSize const superviewSize = self.superview.size;
  BOOL const isLandscape = superviewSize.width > superviewSize.height;
  CGFloat const minHeight = (isLandscape ? 0.6 : 0.3) * superviewSize.height;
  UIFont * font = self.distanceLabel.font;
  UILabel * label = self.titleLabel;
  for (CGFloat size = fontSize; size >= minFontSize; size -= 1.0)
  {
    label.font = [font fontWithSize:size];
    label.width = width;
    [label sizeToFit];
    if (label.height <= minHeight)
      break;
  }
}

- (void)resizeTypeAndDistanceWithWidth:(CGFloat)width
{
  self.typeLabel.width = self.distanceLabel.width = width;
  [self.typeLabel sizeToFit];
  [self.distanceLabel sizeToFit];
}

- (void)setDirectionArrowTransform:(CGAffineTransform)transform
{
  self.directionArrow.transform = transform;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  // Prevent super call to stop event propagation
  // [super touchesBegan:touches withEvent:event];
  [self removeFromSuperview];
}

@end
