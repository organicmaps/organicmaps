#import "MWMPPPreviewLayoutHelper.h"
#import "Common.h"
#import "MWMDirectionView.h"
#import "MWMPlacePageData.h"
#import "MWMTableViewCell.h"
#import "UIColor+MapsmeColor.h"

#include "std/array.hpp"

namespace
{
array<NSString *, 7> kPreviewCells = {{@"_MWMPPPTitle", @"_MWMPPPExternalTitle", @"_MWMPPPSubtitle",
  @"_MWMPPPSchedule", @"_MWMPPPBooking", @"_MWMPPPAddress", @"_MWMPPPSpace"}};
}  // namespace

#pragma mark - Base
// Base class for avoiding copy-paste in inheriting cells.
@interface _MWMPPPCellBase : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * distance;
@property(weak, nonatomic) IBOutlet UIImageView * compass;
@property(weak, nonatomic) IBOutlet UIView * distanceView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * trailing;
@property(copy, nonatomic) TMWMVoidBlock tapOnDistance;

@end

@implementation _MWMPPPCellBase

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.separatorInset = {0, self.width / 2, 0, self.width / 2};
}

- (IBAction)tap
{
  if (self.tapOnDistance)
    self.tapOnDistance();
}

@end

#pragma mark - Title

@interface _MWMPPPTitle : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * title;

@end

@implementation _MWMPPPTitle
@end

#pragma mark - External Title

@interface _MWMPPPExternalTitle : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * externalTitle;

@end

@implementation _MWMPPPExternalTitle
@end

#pragma mark - Subtitle

@interface _MWMPPPSubtitle : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * subtitle;

@end

@implementation _MWMPPPSubtitle
@end

#pragma mark - Schedule

@interface _MWMPPPSchedule : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * schedule;

@end

@implementation _MWMPPPSchedule
@end

#pragma mark - Booking

@interface _MWMPPPBooking : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * rating;
@property(weak, nonatomic) IBOutlet UILabel * pricing;

- (void)configWithRating:(NSString *)rating pricing:(NSString *)pricing;

@end

@implementation _MWMPPPBooking

- (void)configWithRating:(NSString *)rating pricing:(NSString *)pricing
{
  self.rating.text = rating;
  self.pricing.text = pricing;
}

@end

#pragma mark - Address

@interface _MWMPPPAddress : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * address;

@end

@implementation _MWMPPPAddress
@end

@interface MWMPPPreviewLayoutHelper ()

@property(weak, nonatomic) UITableView * tableView;

@property(weak, nonatomic) NSLayoutConstraint * distanceCellTrailing;
@property(weak, nonatomic) UIView * distanceView;

@property(nonatomic) CGFloat leading;
@property(nonatomic) MWMDirectionView * directionView;
@property(copy, nonatomic) NSString * distance;
@property(weak, nonatomic) UIImageView * compass;
@property(nonatomic) NSIndexPath * lastCellIndexPath;
@end

@implementation MWMPPPreviewLayoutHelper

- (instancetype)initWithTableView:(UITableView *)tableView
{
  self = [super init];
  if (self)
    _tableView = tableView;

  return self;
}

- (void)registerCells
{
  for (auto name : kPreviewCells)
    [self.tableView registerNib:[UINib nibWithNibName:name bundle:nil] forCellReuseIdentifier:name];
}

- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath withData:(MWMPlacePageData *)data
{
  using namespace place_page;
  auto tableView = self.tableView;
  auto const row = data.previewRows[indexPath.row];
  auto cellName = kPreviewCells[static_cast<size_t>(row)];

  // -2 because last cell is always the spacer cell.
  BOOL const isNeedToShowDistance = (indexPath.row == data.previewRows.size() - 2);

  UITableViewCell * c = [tableView dequeueReusableCellWithIdentifier:cellName];
  switch(row)
  {
  case PreviewRows::Title:
    static_cast<_MWMPPPTitle *>(c).title.text = data.title;
    break;
  case PreviewRows::ExternalTitle:
    static_cast<_MWMPPPExternalTitle *>(c).externalTitle.text = data.externalTitle;
    break;
  case PreviewRows::Subtitle:
    static_cast<_MWMPPPSubtitle *>(c).subtitle.text = data.subtitle;
    break;
  case PreviewRows::Schedule:
  {
    auto scheduleCell = static_cast<_MWMPPPSchedule *>(c);
    switch (data.schedule)
    {
    case place_page::OpeningHours::AllDay:
      scheduleCell.schedule.text = L(@"twentyfour_seven");
      scheduleCell.schedule.textColor = [UIColor blackSecondaryText];
      break;
    case place_page::OpeningHours::Open:
      scheduleCell.schedule.text = L(@"editor_time_open");
      scheduleCell.schedule.textColor = [UIColor blackSecondaryText];
      break;
    case place_page::OpeningHours::Closed:
      scheduleCell.schedule.text = L(@"closed_now");
      scheduleCell.schedule.textColor = [UIColor red];
      break;
    case place_page::OpeningHours::Unknown: NSAssert(false, @"Incorrect schedule!"); break;
    }
    break;
  }
  case PreviewRows::Booking:
  {
    auto bookingCell = static_cast<_MWMPPPBooking *>(c);
    [bookingCell configWithRating:data.bookingRating pricing:data.bookingApproximatePricing];
    [data assignOnlinePriceToLabel:bookingCell.pricing];
    return c;
  }
  case PreviewRows::Address:
    static_cast<_MWMPPPAddress *>(c).address.text = data.address;
    break;
  case PreviewRows::Space:
    return c;
  }

  auto baseCell = static_cast<_MWMPPPCellBase *>(c);
  if (isNeedToShowDistance)
  {
    [self showDistanceOnCell:baseCell withData:data];
    self.lastCellIndexPath = indexPath;
  }
  else
    [self hideDistanceOnCell:baseCell];

  return c;
}

- (void)showDistanceOnCell:(_MWMPPPCellBase *)cell withData:(MWMPlacePageData *)data
{
  cell.trailing.priority = UILayoutPriorityDefaultLow;
  cell.distance.text = self.distance;
  cell.tapOnDistance = ^{
    [self.directionView show];
  };
  [cell.contentView setNeedsLayout];
  self.compass = cell.compass;
  self.distanceCellTrailing = cell.trailing;
  self.distanceView = cell.distanceView;
  cell.distanceView.hidden = NO;

  auto dv = self.directionView;
  dv.titleLabel.text = data.title;
  dv.typeLabel.text = data.subtitle;
  dv.distanceLabel.text = self.distance;
}

- (void)hideDistanceOnCell:(_MWMPPPCellBase *)cell
{
  cell.trailing.priority = UILayoutPriorityDefaultHigh;
  [cell.contentView setNeedsLayout];
  cell.distanceView.hidden = YES;
}

- (void)rotateDirectionArrowToAngle:(CGFloat)angle
{
  auto const t = CATransform3DMakeRotation(M_PI_2 - angle, 0, 0, 1);
  self.compass.layer.transform = t;
  self.directionView.directionArrow.layer.transform = t;
}

- (void)setDistanceToObject:(NSString *)distance
{
  if (!distance.length)
    return;

  if ([self.distance isEqualToString:distance])
    return;

  self.distance = distance;
  self.directionView.distanceLabel.text = distance;
}

- (CGFloat)height
{
  auto const rect = [self.tableView rectForRowAtIndexPath:self.lastCellIndexPath];
  return rect.origin.y + rect.size.height;
}

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] init];
  return _directionView;
}

@end
