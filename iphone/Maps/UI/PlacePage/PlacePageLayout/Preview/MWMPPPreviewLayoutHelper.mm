#import "MWMPPPreviewLayoutHelper.h"
#import <Crashlytics/Crashlytics.h>
#import "MWMCommon.h"
#import "MWMDirectionView.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageManagerHelper.h"
#import "MWMTableViewCell.h"
#import "MWMUGCViewModel.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include <array>

#pragma mark - Base
// Base class for avoiding copy-paste in inheriting cells.
@interface _MWMPPPCellBase : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * distance;
@property(weak, nonatomic) IBOutlet UIImageView * compass;
@property(weak, nonatomic) IBOutlet UIView * distanceView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * trailing;
@property(copy, nonatomic) MWMVoidBlock tapOnDistance;

@end

@implementation _MWMPPPCellBase

- (void)layoutSubviews
{
  [super layoutSubviews];
  auto const inset = self.width / 2;
  self.separatorInset = {0, inset, 0, inset};
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
@property(weak, nonatomic) IBOutlet UIView * popular;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleTrailing;

@end

@implementation _MWMPPPTitle

- (void)configWithTitle:(NSString *)title popular:(BOOL)popular
{
  self.title.text = title;
  self.titleTrailing.priority = popular ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.popular.hidden = !popular;
  [self setNeedsLayout];
}

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

#pragma mark - Address

@interface _MWMPPPAddress : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * address;

@end

@implementation _MWMPPPAddress
@end

@interface _MWMPPPSpace : _MWMPPPCellBase
@end

@implementation _MWMPPPSpace
@end

namespace
{
std::array<Class, 9> const kPreviewCells = {{[_MWMPPPTitle class],
                                        [_MWMPPPExternalTitle class],
                                        [_MWMPPPSubtitle class],
                                        [_MWMPPPSchedule class],
                                        [MWMPPPReview class],
                                        [MWMPPPSearchSimilarButton class],
                                        [_MWMPPPAddress class],
                                        [_MWMPPPSpace class],
                                        [MWMAdBanner class]}};
}  // namespace

@interface MWMPPPreviewLayoutHelper ()

@property(copy, nonatomic) NSString * distance;
@property(copy, nonatomic) NSString * speedAndAltitude;
@property(nonatomic) MWMDirectionView * directionView;
@property(nonatomic) NSUInteger distanceRow;
@property(weak, nonatomic) MWMAdBanner * cachedBannerCell;
@property(weak, nonatomic) MWMPlacePageData * data;
@property(weak, nonatomic) NSLayoutConstraint * distanceCellTrailing;
@property(weak, nonatomic) UIImageView * compass;
@property(weak, nonatomic) UITableView * tableView;
@property(weak, nonatomic) UIView * distanceView;
@property(weak, nonatomic) _MWMPPPSubtitle * cachedSubtitle;
@property(weak, nonatomic) id<MWMPPPreviewLayoutHelperDelegate> delegate;

@end

@implementation MWMPPPreviewLayoutHelper

- (instancetype)initWithTableView:(UITableView *)tableView
{
  self = [super init];
  if (self)
  {
    _tableView = tableView;
    [self registerCells];
  }

  return self;
}

- (void)registerCells
{
  for (Class cls : kPreviewCells)
    [self.tableView registerWithCellClass:cls];
}

- (void)configWithData:(MWMPlacePageData *)data
{
  self.data = data;
  auto const & previewRows = data.previewRows;
  using place_page::PreviewRows;

  if (data.isMyPosition || previewRows.size() == 1)
  {
    self.distanceRow = 0;
  }
  else
  {
    auto it = find(previewRows.begin(), previewRows.end(), PreviewRows::Address);
    if (it == previewRows.end())
      it = find(previewRows.begin(), previewRows.end(), PreviewRows::Subtitle);
    if (it != previewRows.end())
      self.distanceRow = distance(previewRows.begin(), it);
  }
}

- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath withData:(MWMPlacePageData *)data
{
  using namespace place_page;
  auto tableView = self.tableView;
  auto const row = data.previewRows[indexPath.row];
  Class cls = kPreviewCells[base::Key(row)];

  auto * c = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  switch(row)
  {
  case PreviewRows::Title:
    [static_cast<_MWMPPPTitle *>(c) configWithTitle:data.title popular:data.isPopular];
    break;
  case PreviewRows::ExternalTitle:
    static_cast<_MWMPPPExternalTitle *>(c).externalTitle.text = data.externalTitle;
    break;
  case PreviewRows::Subtitle:
  {
    auto subtitleCell = static_cast<_MWMPPPSubtitle *>(c);
    if (data.isMyPosition)
      subtitleCell.subtitle.text = self.speedAndAltitude;
    else
      subtitleCell.subtitle.text = data.subtitle;

    self.cachedSubtitle = subtitleCell;
    break;
  }
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
  case PreviewRows::Review:
  {
    auto reviewCell = static_cast<MWMPPPReview *>(c);
    if (data.isBooking)
    {
      auto configCellBlock = ^(MWMPlacePageData *data) {
        [reviewCell configWithRating:data.bookingRating
                        canAddReview:NO
                    isReviewedByUser:NO
                        reviewsCount:0
                        ratingsCount:0
                               price:data.bookingPricing
                            discount:data.bookingDiscount
                           smartDeal:data.isSmartDeal
                         onAddReview:nil];
      };
      configCellBlock(data);
      __weak __typeof(data) weakData = data;
      data.bookingDataUpdatedCallback = ^{
        __strong __typeof(weakData) data = weakData;
        if (data)
          configCellBlock(data);
      };
    }
    else
    {
      NSAssert(data.ugc, @"");
      [reviewCell configWithRating:data.ugc.summaryRating
                      canAddReview:YES
                      isReviewedByUser:!data.ugc.isUGCUpdateEmpty
                      reviewsCount:data.ugc.numberOfReviews
                      ratingsCount:data.ugc.numberOfRatings
                             price:@""
                          discount:0
                         smartDeal:NO
                       onAddReview:^{
                         [MWMPlacePageManagerHelper showUGCAddReview:MWMRatingSummaryViewValueTypeNoValue
                                                         fromSource:MWMUGCReviewSourcePlacePagePreview];
                       }];
    }
    return reviewCell;
  }
  case PreviewRows::SearchSimilar:
  {
    auto searchCell = static_cast<MWMPPPSearchSimilarButton *>(c);
    [searchCell configWithTap:^{
      [MWMPlacePageManagerHelper searchSimilar];
    }];
    return searchCell;
  }
  case PreviewRows::Address:
    static_cast<_MWMPPPAddress *>(c).address.text = data.address;
    break;
  case PreviewRows::Space:
    return c;
  case PreviewRows::Banner:
    auto bannerCell = static_cast<MWMAdBanner *>(c);
    [bannerCell configWithAd:data.nativeAd
               containerType:MWMAdBannerContainerTypePlacePage
                canRemoveAds:[SubscriptionManager canMakePayments]
                 onRemoveAds: ^{
      [[MapViewController sharedController] showRemoveAds];
    }];

    self.cachedBannerCell = bannerCell;
    return bannerCell;
  }

  auto baseCell = static_cast<_MWMPPPCellBase *>(c);

  if (indexPath.row == self.distanceRow)
    [self showDistanceOnCell:baseCell withData:data];
  else
    [self hideDistanceOnCell:baseCell];

  return c;
}

- (void)showDistanceOnCell:(_MWMPPPCellBase *)cell withData:(MWMPlacePageData *)data
{
  cell.trailing.priority = UILayoutPriorityDefaultLow;
  cell.distance.text = self.distance;
  auto directionView = self.directionView;
  cell.tapOnDistance = ^{
    [directionView show];
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
  if (!distance.length || [self.distance isEqualToString:distance])
    return;

  self.distance = distance;
  self.directionView.distanceLabel.text = distance;
}

- (void)setSpeedAndAltitude:(NSString *)speedAndAltitude
{
  auto data = self.data;
  if (!data)
    return;
  if ([speedAndAltitude isEqualToString:_speedAndAltitude] || !data.isMyPosition)
    return;

  _speedAndAltitude = speedAndAltitude;
  self.cachedSubtitle.subtitle.text = speedAndAltitude;
}

- (void)insertRowAtTheEnd
{
  auto data = self.data;
  if (!data)
    return;
  [self.tableView insertRowsAtIndexPaths:@[ self.lastCellIndexPath ]
                        withRowAnimation:UITableViewRowAnimationLeft];
  [self.delegate heightWasChanged];
}

- (void)notifyHeightWashChanded { [self.delegate heightWasChanged]; }

- (CGFloat)height
{
  auto const rect = [self.tableView rectForRowAtIndexPath:self.lastCellIndexPath];
  auto const height = rect.origin.y + rect.size.height;
  if (!self.lastCellIndexPath)
    return height;

  auto constexpr gapBannerHeight = 4.0;
  CGFloat const excessHeight = self.cachedBannerCell.state == MWMAdBannerStateDetailed
                                   ? [MWMAdBanner detailedBannerExcessHeight]
                                   : 0;

  return height + gapBannerHeight - excessHeight;
}

- (void)layoutInOpenState:(BOOL)isOpen
{
  if (IPAD)
    return;
  dispatch_async(dispatch_get_main_queue(), ^{
    auto data = self.data;
    if (!data)
      return;

    [self.tableView update:^{
      self.cachedBannerCell.state = isOpen ? MWMAdBannerStateDetailed : MWMAdBannerStateCompact;
    }];
  });
}

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] init];
  return _directionView;
}

- (NSIndexPath *)lastCellIndexPath
{
  auto data = self.data;
  if (!data)
    return nil;
  return [NSIndexPath indexPathForRow:data.previewRows.size() - 1
                            inSection:static_cast<NSUInteger>(place_page::Sections::Preview)];
}

- (BOOL)lastCellIsBanner
{
  auto data = self.data;
  if (!data)
    return NO;
  return data.previewRows.back() == place_page::PreviewRows::Banner;
}

@end
