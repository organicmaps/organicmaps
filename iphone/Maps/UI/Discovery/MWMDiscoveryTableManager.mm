#import "MWMDiscoveryTableManager.h"
#import "MWMDiscoveryTapDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "DiscoveryControllerViewModel.hpp"

#include "map/place_page_info.hpp"

#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "search/result.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <tuple>

using namespace std;

namespace discovery
{
NSString * StatProvider(ItemType const type)
{
  switch (type)
  {
  case ItemType::Viator: return kStatViator;
  case ItemType::LocalExperts: return kStatLocalsProvider;
  case ItemType::Attractions: return kStatSearchAttractions;
  case ItemType::Cafes: return kStatSearchRestaurants;
  case ItemType::Hotels: return kStatBooking;
  }
}

pair<NSString *, MWMRatingSummaryViewValueType> FormattedRating(float const rawValue)
{
  auto const str = place_page::rating::GetRatingFormatted(rawValue);
  auto const impress = static_cast<MWMRatingSummaryViewValueType>(place_page::rating::GetImpress(rawValue));
  return {@(str.c_str()), impress};
};
}  // namespace discovery

using namespace discovery;

namespace
{
auto const kDefaultRowAnimation = UITableViewRowAnimationFade;

string GetDistance(m2::PointD const & from, m2::PointD const & to)
{
  string distance;
  auto const f = MercatorBounds::ToLatLon(from);
  auto const t = MercatorBounds::ToLatLon(to);
  measurement_utils::FormatDistance(ms::DistanceOnEarth(f.lat, f.lon, t.lat, t.lon), distance);
  return distance;
}
}  // namespace

@interface MWMDiscoveryCollectionView : UICollectionView
@property(nonatomic) ItemType itemType;
@end

@implementation MWMDiscoveryCollectionView
@end

@interface MWMDiscoveryTableManager ()<UITableViewDataSource, UICollectionViewDelegate,
                                       UICollectionViewDataSource>
{
  vector<ItemType> m_types;
  vector<ItemType> m_loadingTypes;
  vector<ItemType> m_failedTypes;
}

@property(weak, nonatomic) UITableView * tableView;
@property(nonatomic) GetModelCallback model;
@property(weak, nonatomic) id<MWMDiscoveryTapDelegate> delegate;

@end

@implementation MWMDiscoveryTableManager

#pragma mark - Public

- (instancetype)initWithTableView:(UITableView *)tableView
                         delegate:(id<MWMDiscoveryTapDelegate>)delegate
                            model:(GetModelCallback &&)modelCallback
{
  self = [super init];
  if (self)
  {
    _tableView = tableView;
    _delegate = delegate;
    _model = move(modelCallback);
    tableView.dataSource = self;
    tableView.rowHeight = UITableViewAutomaticDimension;
    tableView.estimatedRowHeight = 218;
    if (@available(iOS 11.0, *))
      tableView.insetsContentViewsToSafeArea = NO;
    [self registerCells];
  }
  return self;
}

- (void)loadItems:(vector<ItemType> const &)types
{
  m_types = types;
  m_loadingTypes = types;
  [self.tableView reloadData];
}

- (void)reloadItem:(ItemType const)type
{
  if (self.model().GetItemsCount(type) == 0)
  {
    [self removeItem:type];
    return;
  }

  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type),
                       m_loadingTypes.end());
  m_failedTypes.erase(remove(m_failedTypes.begin(), m_failedTypes.end(), type),
                      m_failedTypes.end());
  auto const position = [self position:type];
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:0 inSection:position]]
                        withRowAnimation:kDefaultRowAnimation];

  [Statistics logEvent:kStatPlacepageSponsoredShow
        withParameters:@{
          kStatProvider: StatProvider(type),
          kStatPlacement: kStatDiscovery,
          kStatState: self.isOnline ? kStatOnline : kStatOffline
        }];
}

- (void)errorAtItem:(ItemType const)type
{
  CHECK(type == ItemType::Viator || type == ItemType::LocalExperts,
        ("Error on item with type:", static_cast<int>(type)));
  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type),
                       m_loadingTypes.end());
  m_failedTypes.push_back(type);
  auto const position = [self position:type];

  [Statistics logEvent:kStatPlacepageSponsoredError
        withParameters:@{kStatProvider: StatProvider(type), kStatPlacement: kStatDiscovery}];

  [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:position]
                withRowAnimation:kDefaultRowAnimation];
}

#pragma mark - Private

- (BOOL)isOnline { return find(m_types.begin(), m_types.end(), ItemType::Viator) != m_types.end(); }

- (void)removeItem:(ItemType const)type
{
  auto const position = [self position:type];
  m_types.erase(remove(m_types.begin(), m_types.end(), type), m_types.end());
  m_failedTypes.erase(remove(m_failedTypes.begin(), m_failedTypes.end(), type),
                      m_failedTypes.end());
  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type),
                       m_loadingTypes.end());

  auto indexSet = [NSIndexSet indexSetWithIndex:position];
  auto tv = self.tableView;
  if (m_types.empty())
    [tv reloadSections:indexSet withRowAnimation:kDefaultRowAnimation];
  else
    [tv deleteSections:indexSet withRowAnimation:kDefaultRowAnimation];
}

- (void)registerCells
{
  auto tv = self.tableView;
  [tv registerWithCellClass:[MWMDiscoverySpinnerCell class]];
  [tv registerWithCellClass:[MWMDiscoveryOnlineTemplateCell class]];
  [tv registerWithCellClass:[MWMDiscoverySearchCollectionHolderCell class]];
  [tv registerWithCellClass:[MWMDiscoveryViatorCollectionHolderCell class]];
  [tv registerWithCellClass:[MWMDiscoveryLocalExpertCollectionHolderCell class]];
  [tv registerWithCellClass:[MWMDiscoveryBookingCollectionHolderCell class]];
  [tv registerWithCellClass:[MWMDiscoveryNoResultsCell class]];
}

- (NSInteger)position:(ItemType const)type
{
  auto const it = find(m_types.begin(), m_types.end(), type);
  if (it == m_types.end())
    CHECK(false, ("Incorrect item type:", static_cast<int>(type)));

  return distance(m_types.begin(), it);
}

- (MWMDiscoverySearchCollectionHolderCell *)searchCollectionHolderCell:(NSIndexPath *)indexPath
{
  Class cls = [MWMDiscoverySearchCollectionHolderCell class];
  auto const type = m_types[indexPath.section];
  auto cell = static_cast<MWMDiscoverySearchCollectionHolderCell *>(
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  auto collection = static_cast<MWMDiscoveryCollectionView *>(cell.collectionView);
  switch (type)
  {
  case ItemType::Attractions: [cell configAttractionsCell]; break;
  case ItemType::Cafes: [cell configCafesCell]; break;
  default: NSAssert(false, @""); return nil;
  }
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = type;
  return cell;
}

- (MWMDiscoveryViatorCollectionHolderCell *)viatorCollectionHolderCell:(NSIndexPath *)indexPath
{
  Class cls = [MWMDiscoveryViatorCollectionHolderCell class];
  auto cell = static_cast<MWMDiscoveryViatorCollectionHolderCell *>(
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  auto collection = static_cast<MWMDiscoveryCollectionView *>(cell.collectionView);
  [cell configWithTap:^{
    [self.delegate tapOnLogo:ItemType::Viator];
  }];

  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = ItemType::Viator;
  return cell;
}

- (MWMDiscoveryLocalExpertCollectionHolderCell *)localExpertsCollectionHolderCell:
    (NSIndexPath *)indexPath
{
  Class cls = [MWMDiscoveryLocalExpertCollectionHolderCell class];
  auto cell = static_cast<MWMDiscoveryLocalExpertCollectionHolderCell *>(
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  auto collection = static_cast<MWMDiscoveryCollectionView *>(cell.collectionView);
  [cell config];
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = ItemType::LocalExperts;
  return cell;
}

- (MWMDiscoveryBookingCollectionHolderCell *)bookingCollectionHolderCell:(NSIndexPath *)indexPath
{
  Class cls = [MWMDiscoveryBookingCollectionHolderCell class];
  auto cell = static_cast<MWMDiscoveryBookingCollectionHolderCell *>(
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  auto collection = static_cast<MWMDiscoveryCollectionView *>(cell.collectionView);
  [cell config];
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = ItemType::Hotels;
  return cell;
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  auto constexpr kNumberOfRows = 1;
  return kNumberOfRows;
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (m_types.empty())
  {
    Class cls = [MWMDiscoveryNoResultsCell class];
    return static_cast<MWMDiscoveryNoResultsCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  }

  auto const type = m_types[indexPath.section];
  bool const isFailed =
      find(m_failedTypes.begin(), m_failedTypes.end(), type) != m_failedTypes.end();
  bool const isLoading =
      find(m_loadingTypes.begin(), m_loadingTypes.end(), type) != m_loadingTypes.end();

  switch (type)
  {
  case ItemType::Viator:
  case ItemType::LocalExperts:
  {
    if (isLoading || isFailed)
    {
      Class cls = [MWMDiscoveryOnlineTemplateCell class];
      auto cell = static_cast<MWMDiscoveryOnlineTemplateCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [cell configWithType:type == ItemType::Viator ? MWMDiscoveryOnlineTemplateTypeViator :
                                                      MWMDiscoveryOnlineTemplateTypeLocals
               needSpinner:isLoading
                       tap:^{
                         [self.delegate openURLForItem:type];
                       }];
      return cell;
    }
    return type == ItemType::Viator ? [self viatorCollectionHolderCell:indexPath]
                                    : [self localExpertsCollectionHolderCell:indexPath];
  }
  case ItemType::Attractions:
  case ItemType::Cafes:
  {
    if (isLoading)
    {
      Class cls = [MWMDiscoverySpinnerCell class];
      auto cell = static_cast<MWMDiscoverySpinnerCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      return cell;
    }
    return [self searchCollectionHolderCell:indexPath];
  }
  case ItemType::Hotels: return [self bookingCollectionHolderCell:indexPath];
  }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return static_cast<NSInteger>(MAX(m_types.size(), 1));
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(MWMDiscoveryCollectionView *)collectionView
    didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
  [self.delegate tapOnItem:collectionView.itemType atIndex:indexPath.row];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(MWMDiscoveryCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
  return self.model().GetItemsCount(collectionView.itemType);
}

- (UICollectionViewCell *)collectionView:(MWMDiscoveryCollectionView *)collectionView
                  cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  auto const type = collectionView.itemType;
  auto const & model = self.model();
  NSString * ratingValue;
  MWMRatingSummaryViewValueType ratingType;
  switch (type)
  {
  case ItemType::Attractions:
  case ItemType::Cafes:
  {
    Class cls = [MWMDiscoverySearchCell class];
    auto cell = static_cast<MWMDiscoverySearchCell *>(
        [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const & sr = type == ItemType::Attractions ? model.GetAttractionAt(indexPath.row)
                                                    : model.GetCafeAt(indexPath.row);
    auto const & pt = type == ItemType::Attractions ? model.GetAttractionReferencePoint()
                                                    : model.GetCafeReferencePoint();
    [cell configWithTitle:@(sr.GetString().c_str())
                 subtitle:@(sr.GetFeatureTypeName().c_str())
                 distance:@(GetDistance(pt, sr.GetFeatureCenter()).c_str())
                      tap:^{
                        [self.delegate routeToItem:type atIndex:indexPath.row];
                      }];
    return cell;
  }
  case ItemType::Viator:
  {
    Class cls = [MWMViatorElement class];
    auto cell = static_cast<MWMViatorElement *>(
        [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const & v = model.GetViatorAt(indexPath.row);
    auto imageURL = [NSURL URLWithString:@(v.m_photoUrl.c_str())];
    auto pageURL = [NSURL URLWithString:@(v.m_pageUrl.c_str())];

    tie(ratingValue, ratingType) = FormattedRating(v.m_rating);

    auto viatorModel = [[MWMViatorItemModel alloc]
        initWithImageURL:imageURL
                 pageURL:pageURL
                   title:@(v.m_title.c_str())
         ratingFormatted:ratingValue
              ratingType:ratingType
                duration:@(v.m_duration.c_str())
                   price:@(v.m_priceFormatted.c_str())];
    cell.model = viatorModel;
    return cell;
  }
  case ItemType::LocalExperts:
  {
    Class cls = [MWMDiscoveryLocalExpertCell class];
    auto cell = static_cast<MWMDiscoveryLocalExpertCell *>(
        [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const & expert = model.GetExpertAt(indexPath.row);
    tie(ratingValue, ratingType) = FormattedRating(expert.m_rating);

    [cell configWithAvatarURL:@(expert.m_photoUrl.c_str())
                         name:@(expert.m_name.c_str())
                  ratingValue:ratingValue
                   ratingType:ratingType
                        price:expert.m_pricePerHour
                     currency:@(expert.m_currency.c_str())
                          tap:^{
                            [self.delegate tapOnItem:type atIndex:indexPath.row];
                          }];
    return cell;
  }
  case ItemType::Hotels:
  {
    Class cls = [MWMDiscoveryBookingCell class];
    auto cell = static_cast<MWMDiscoveryBookingCell *>(
        [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const & sr = model.GetHotelAt(indexPath.row);
    auto const & pt = model.GetHotelReferencePoint();

    NSMutableString * subtitle = nil;
    auto starsCount = sr.GetStarsCount();
    if (starsCount == 0)
    {
      subtitle = [@(sr.GetFeatureTypeName().c_str()) mutableCopy];
    }
    else
    {
      subtitle = [@"" mutableCopy];
      for (int i = 0; i < starsCount; ++i)
        [subtitle appendString:@"★"];
    }

    tie(ratingValue, ratingType) = FormattedRating(sr.GetHotelRating());

    [cell configWithAvatarURL:nil
                        title:@(sr.GetString().c_str())
                     subtitle:[subtitle copy]
                        price:@(sr.GetHotelApproximatePricing().c_str())
                  ratingValue:ratingValue
                   ratingType:ratingType
                     distance:@(GetDistance(pt, sr.GetFeatureCenter()).c_str())
                 onBuildRoute:^{
                   [self.delegate routeToItem:type atIndex:indexPath.row];
                 }];
    return cell;
  }
  }
}

@end
