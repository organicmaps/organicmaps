#import "MWMPlacePagePreviewCell.h"
#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMDirectionView.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayout.h"
#import "UIColor+MapsmeColor.h"

#include "std/array.hpp"
#include "std/vector.hpp"

namespace
{
array<NSString *, 6> kPPPClasses = {{@"_MWMPPPTitle", @"_MWMPPPExternalTitle", @"_MWMPPPSubtitle",
                                     @"_MWMPPPSchedule", @"_MWMPPPBooking", @"_MWMPPPAddress"}};

enum class Labels
{
  Title,
  ExternalTitle,
  Subtitle,
  Schedule,
  Booking,
  Address
};

void * kContext = &kContext;
NSString * const kTableViewContentSizeKeyPath = @"contentSize";
CGFloat const kDefaultTableViewLeading = 16;
CGFloat const kCompressedTableViewLeading = 56;

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

@end

@implementation _MWMPPPBooking
@end

#pragma mark - Address

@interface _MWMPPPAddress : _MWMPPPCellBase

@property(weak, nonatomic) IBOutlet UILabel * address;

@end

@implementation _MWMPPPAddress
@end

#pragma mark - Public

@interface MWMPlacePagePreviewCell ()<UITableViewDelegate, UITableViewDataSource,
                                      MWMCircularProgressProtocol>
{
  vector<Labels> m_cells;
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;
@property(weak, nonatomic) NSLayoutConstraint * trailing;
@property(weak, nonatomic) UIView * distanceView;

@property(weak, nonatomic) IBOutlet UIView * downloaderParentView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewLeading;

@property(nonatomic) MWMCircularProgress * mapDownloadProgress;

@property(nonatomic) BOOL isDirectionViewAvailable;

@property(weak, nonatomic) MWMPlacePageData * data;
@property(weak, nonatomic) id<MWMPlacePageCellUpdateProtocol> delegate;
@property(weak, nonatomic) id<MWMPlacePageLayoutDataSource> dataSource;

@property(copy, nonatomic) NSString * distance;
@property(weak, nonatomic) UIImageView * compass;
@property(nonatomic) CGFloat currentContentHeight;

@property(nonatomic) MWMDirectionView * directionView;

@property(copy, nonatomic) TMWMVoidBlock tapAction;

@end

@implementation MWMPlacePagePreviewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  for (auto const s : kPPPClasses)
    [self.tableView registerNib:[UINib nibWithNibName:s bundle:nil] forCellReuseIdentifier:s];

  self.tableView.estimatedRowHeight = 20;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  [self registerObserver];

  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap)];
  [self addGestureRecognizer:tap];
}

- (void)dealloc { [self unregisterObserver]; }
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat const height = s.CGSizeValue.height;
    if (!equalScreenDimensions(height, self.currentContentHeight))
    {
      self.currentContentHeight = height;
      self.tableViewHeight.constant = height;
      [self setNeedsLayout];
      [self.delegate updateCellWithForceReposition:YES];
    }
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] initWithManager:nil];
  return _directionView;
}

- (void)setIsDirectionViewAvailable:(BOOL)isDirectionViewAvailable
{
  if (_isDirectionViewAvailable == isDirectionViewAvailable)
    return;
  _isDirectionViewAvailable = isDirectionViewAvailable;
  [self setNeedsLayout];
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
  {
    self.isDirectionViewAvailable = NO;
    [self setNeedsLayout];
    return;
  }

  if ([self.distance isEqualToString:distance])
    return;

  self.distance = distance;
  self.directionView.distanceLabel.text = distance;
  self.isDirectionViewAvailable = YES;
}

- (void)unregisterObserver
{
  [self.tableView removeObserver:self forKeyPath:kTableViewContentSizeKeyPath context:kContext];
}

- (void)registerObserver
{
  [self.tableView addObserver:self
                   forKeyPath:kTableViewContentSizeKeyPath
                      options:NSKeyValueObservingOptionNew
                      context:kContext];
}

- (void)setDownloadingProgress:(CGFloat)progress { self.mapDownloadProgress.progress = progress; }
- (void)setDownloaderViewHidden:(BOOL)isHidden animated:(BOOL)isAnimated
{
  self.downloaderParentView.hidden = isHidden;
  self.tableViewLeading.constant =
      isHidden ? kDefaultTableViewLeading : kCompressedTableViewLeading;
  [self setNeedsLayout];

  if (!isHidden)
    self.mapDownloadProgress.state = MWMCircularProgressStateNormal;

  if (!isAnimated)
    return;

  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     [self layoutIfNeeded];
                   }];
}

- (void)configure:(MWMPlacePageData *)data
    updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
              dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource
               tapAction:(TMWMVoidBlock)tapAction
{
  self.data = data;
  self.delegate = delegate;
  self.dataSource = dataSource;
  [self setDistanceToObject:dataSource.distanceToObject];

  m_cells.clear();

  if (data.title.length)
    m_cells.push_back(Labels::Title);

  if (data.externalTitle.length)
    m_cells.push_back(Labels::ExternalTitle);

  if (data.subtitle.length)
    m_cells.push_back(Labels::Subtitle);

  if (data.schedule != place_page::OpeningHours::Unknown)
    m_cells.push_back(Labels::Schedule);

  if (data.bookingRating.length)
    m_cells.push_back(Labels::Booking);

  if (data.address.length)
    m_cells.push_back(Labels::Address);

  [self.tableView reloadData];

  NSAssert(tapAction, @"Cell must be tappable!");
  self.tapAction = tapAction;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_cells.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto data = self.data;
  _MWMPPPCellBase * c = nil;
  BOOL const isNeedToShowDistance =
      self.isDirectionViewAvailable && (indexPath.row == m_cells.size() - 1);

  switch (m_cells[indexPath.row])
  {
  case Labels::Title:
  {
    c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPTitle className]];
    static_cast<_MWMPPPTitle *>(c).title.text = data.title;
    break;
  }
  case Labels::ExternalTitle:
  {
    c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPExternalTitle className]];
    static_cast<_MWMPPPExternalTitle *>(c).externalTitle.text = data.externalTitle;
    break;
  }
  case Labels::Subtitle:
  {
    c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPSubtitle className]];
    static_cast<_MWMPPPSubtitle *>(c).subtitle.text = data.subtitle;
    break;
  }
  case Labels::Schedule:
  {
    c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPSchedule className]];
    auto castedCell = static_cast<_MWMPPPSchedule *>(c);
    switch (data.schedule)
    {
    case place_page::OpeningHours::AllDay:
      castedCell.schedule.text = L(@"twentyfour_seven");
      castedCell.schedule.textColor = [UIColor blackSecondaryText];
      break;
    case place_page::OpeningHours::Open:
      castedCell.schedule.text = L(@"editor_time_open");
      castedCell.schedule.textColor = [UIColor blackSecondaryText];
      break;
    case place_page::OpeningHours::Closed:
      castedCell.schedule.text = L(@"closed_now");
      castedCell.schedule.textColor = [UIColor red];
      break;
    case place_page::OpeningHours::Unknown: NSAssert(false, @"Incorrect schedule!"); break;
    }
    break;
  }
  case Labels::Booking:
  {
    _MWMPPPBooking * c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPBooking className]];
    c.rating.text = data.bookingRating;
    c.pricing.text = data.bookingApproximatePricing;
    [data assignOnlinePriceToLabel:c.pricing];
    return c;
  }
  case Labels::Address:
  {
    c = [tableView dequeueReusableCellWithIdentifier:[_MWMPPPAddress className]];
    static_cast<_MWMPPPAddress *>(c).address.text = data.address;
    break;
  }
  }

  if (isNeedToShowDistance)
    [self showDistanceOnCell:c];
  else
    [self hideDistanceOnCell:c];

  return c;
}

- (void)showDistanceOnCell:(_MWMPPPCellBase *)cell
{
  cell.trailing.priority = UILayoutPriorityDefaultLow;
  cell.distance.text = self.distance;
  cell.tapOnDistance = ^{
    [self.directionView show];
  };
  [cell.contentView setNeedsLayout];
  self.compass = cell.compass;
  self.trailing = cell.trailing;
  self.distanceView = cell.distanceView;
  cell.distanceView.hidden = NO;
}

- (void)hideDistanceOnCell:(_MWMPPPCellBase *)cell
{
  cell.trailing.priority = UILayoutPriorityDefaultHigh;
  [cell.contentView setNeedsLayout];
  cell.distanceView.hidden = YES;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(MWMCircularProgress *)progress
{
  [self.dataSource downloadSelectedArea];
}

#pragma mark - Properties

- (MWMCircularProgress *)mapDownloadProgress
{
  if (!_mapDownloadProgress)
  {
    _mapDownloadProgress =
        [MWMCircularProgress downloaderProgressForParentView:self.downloaderParentView];
    _mapDownloadProgress.delegate = self;

    MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
                                                        MWMCircularProgressStateSelected};

    [_mapDownloadProgress setImage:[UIImage imageNamed:@"ic_download"] forStates:affectedStates];
    [_mapDownloadProgress setColoring:MWMButtonColoringBlue forStates:affectedStates];
  }
  return _mapDownloadProgress;
}

#pragma mark - Tap

- (void)tap
{
  if (self.tapAction)
    self.tapAction();
}

@end
