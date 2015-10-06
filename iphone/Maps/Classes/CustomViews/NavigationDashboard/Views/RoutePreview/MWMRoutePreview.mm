#import "Common.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMRoutePointCell.h"
#import "MWMRoutePointLayout.h"
#import "MWMRoutePreview.h"
#import "TimeUtils.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

static NSDictionary * const kEtaAttributes = @{NSForegroundColorAttributeName : UIColor.blackPrimaryText,
                                                          NSFontAttributeName : UIFont.medium17};

@interface MWMRoutePreview () <MWMRoutePointCellDelegate>

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * planningRouteViewHeight;
@property (weak, nonatomic) IBOutlet UIButton * extendButton;
@property (weak, nonatomic) IBOutlet UICollectionView * collectionView;
@property (weak, nonatomic) IBOutlet MWMRoutePointLayout * layout;
@property (weak, nonatomic) IBOutlet UIImageView * arrowImageView;
@property (weak, nonatomic) IBOutlet UIView * planningBox;
@property (weak, nonatomic) IBOutlet UIView * resultsBox;
@property (weak, nonatomic) IBOutlet UIView * errorBox;
@property (weak, nonatomic) IBOutlet UILabel * resultLabel;
@property (weak, nonatomic) IBOutlet UILabel * arriveLabel;
@property (weak, nonatomic) IBOutlet UIImageView * completeImageView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * statusBoxHeight;
@property (nonatomic) UIImageView * movingCellImage;

@end

@implementation MWMRoutePreview

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self setupActualHeight];
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
  [self.collectionView registerNib:[UINib nibWithNibName:[MWMRoutePointCell className] bundle:nil]
        forCellWithReuseIdentifier:[MWMRoutePointCell className]];
}

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  NSString * eta = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
  NSString * resultString = [NSString stringWithFormat:@"%@ • %@ %@",
                             eta,
                             entity.targetDistance,
                             entity.targetUnits];
  NSMutableAttributedString * result = [[NSMutableAttributedString alloc] initWithString:resultString];
  [result addAttributes:kEtaAttributes range:NSMakeRange(0, eta.length)];
  self.resultLabel.attributedText = result;
  if (!IPAD)
    return;

  NSString * arriveStr = [NSDateFormatter localizedStringFromDate:[[NSDate date]
                                                                   dateByAddingTimeInterval:entity.timeToTarget]
                                                        dateStyle:NSDateFormatterNoStyle
                                                        timeStyle:NSDateFormatterShortStyle];
  self.arriveLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"routing_arrive"), arriveStr];
}

- (void)statePlanning
{
  self.resultsBox.hidden = YES;
  self.errorBox.hidden = YES;
  self.planningBox.hidden = NO;
  [self.collectionView reloadData];
  if (!IPAD)
    return;
  [self iPadNotReady];
}

- (void)stateError
{
  self.planningBox.hidden = YES;
  self.resultsBox.hidden = YES;
  self.errorBox.hidden = NO;
  if (!IPAD)
    return;
  [self iPadNotReady];
}

- (void)stateReady
{
  self.planningBox.hidden = YES;
  self.errorBox.hidden = YES;
  self.resultsBox.hidden = NO;
  if (!IPAD)
    return;
  [self iPadReady];
}

- (void)iPadReady
{
  [self layoutIfNeeded];
  self.statusBoxHeight.constant = 76.;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    [self layoutIfNeeded];
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.arriveLabel.alpha = 1.;
    }
    completion:^(BOOL finished)
    {
      self.completeImageView.alpha = 1.;
    }];
  }];
}

- (void)iPadNotReady
{
  self.completeImageView.alpha = 0.;
  self.arriveLabel.alpha = 0.;
  [self layoutIfNeeded];
  self.statusBoxHeight.constant = 56.;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    [self layoutIfNeeded];
  }];
}

- (void)layoutSubviews
{
  [self setupActualHeight];
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  if (!IPAD)
    return super.defaultFrame;
  CGFloat const width = 320.;
  CGFloat const origin = self.isVisible ? 0. : -width;
  return {{origin, self.topBound}, {width, self.height}};
}

- (CGFloat)visibleHeight
{
  CGFloat height = 0.;
  for (UIView * v in self.subviews)
    if (![v isEqual:self.statusbarBackground])
      height += v.height;
  return height;
}

- (void)setRouteBuildingProgress:(CGFloat)progress { }

- (IBAction)extendTap
{
  BOOL const isExtended = !self.extendButton.selected;
  self.extendButton.selected = isExtended;
  [self layoutIfNeeded];
  [self setupActualHeight];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.arrowImageView.transform = isExtended ? CGAffineTransformMakeRotation(M_PI) : CGAffineTransformIdentity;
    [self layoutIfNeeded];
  }];
}

- (void)setupActualHeight
{
  if (IPAD)
  {
    self.height = self.superview.height;
    return;
  }
  BOOL const isPortrait = self.superview.height > self.superview.width;
  CGFloat const height = isPortrait ? 140. : 96.;
  self.planningRouteViewHeight.constant = self.extendButton.selected ? height : 44.;
}

#pragma mark - MWMRoutePointCellDelegate

- (void)didPan:(UIPanGestureRecognizer *)pan cell:(MWMRoutePointCell *)cell
{
  CGPoint const locationPoint = [pan locationInView:self.collectionView];
  static BOOL isNeedToMove;
  static NSIndexPath * indexPathOfMovingCell;

  if (pan.state == UIGestureRecognizerStateBegan)
  {
    self.layout.isNeedToInitialLayout = NO;
    isNeedToMove = NO;
    indexPathOfMovingCell = [self.collectionView indexPathForCell:cell];
    UIGraphicsBeginImageContextWithOptions(cell.bounds.size, NO, 0.);
    [cell drawViewHierarchyInRect:cell.bounds afterScreenUpdates:YES];
    UIImage * image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    self.movingCellImage = [[UIImageView alloc] initWithImage:image];
    [self.collectionView addSubview:self.movingCellImage];
    self.movingCellImage.center = {locationPoint.x - cell.width / 2, locationPoint.y};
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      cell.contentView.alpha = 0.;
      CGFloat const scale = 1.05;
      self.movingCellImage.transform = CGAffineTransformMakeScale(scale, scale);
    }];
  }

  if (pan.state == UIGestureRecognizerStateChanged)
  {
    self.movingCellImage.center = {locationPoint.x - cell.width / 2, locationPoint.y};
    NSIndexPath * finalIndexPath = [self.collectionView indexPathForItemAtPoint:locationPoint];
    if (finalIndexPath && ![finalIndexPath isEqual:indexPathOfMovingCell])
    {
      if (isNeedToMove)
        return;
      isNeedToMove = YES;
      [self.collectionView performBatchUpdates:^
      {
        [self.collectionView moveItemAtIndexPath:finalIndexPath toIndexPath:indexPathOfMovingCell];
        indexPathOfMovingCell = finalIndexPath;
      } completion:nil];
    }
    else
    {
      isNeedToMove = NO;
    }
  }

  if (pan.state == UIGestureRecognizerStateEnded)
  {
    self.layout.isNeedToInitialLayout = YES;
    NSIndexPath * finalIndexPath = [self.collectionView indexPathForItemAtPoint:locationPoint];
    if (finalIndexPath && ![finalIndexPath isEqual:indexPathOfMovingCell])
    {
      isNeedToMove = YES;
      cell.contentView.alpha = 1.;
      [self.collectionView performBatchUpdates:^
      {
        [self.collectionView moveItemAtIndexPath:indexPathOfMovingCell toIndexPath:finalIndexPath];
      }
      completion:^(BOOL finished)
      {
        [self.movingCellImage removeFromSuperview];
        self.movingCellImage.transform = CGAffineTransformIdentity;
      }];
    }
    else
    {
      isNeedToMove = NO;
      cell.contentView.alpha = 1.;
      [self.movingCellImage removeFromSuperview];
    }
  }
}

@end

#pragma mark - UICollectionView

@interface MWMRoutePreview (UICollectionView) <UICollectionViewDataSource, UICollectionViewDelegate>

@end

@implementation MWMRoutePreview (UICollectionView)

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
  return 2;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  MWMRoutePointCell * cell = [collectionView dequeueReusableCellWithReuseIdentifier:[MWMRoutePointCell className] forIndexPath:indexPath];
  cell.number.text = @(indexPath.row + 1).stringValue;
  cell.title.text = !indexPath.row ? @"Current position" : @"";
  cell.delegate = self;
  return cell;
}

@end
