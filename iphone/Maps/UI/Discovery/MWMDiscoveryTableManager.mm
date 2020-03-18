#import "MWMDiscoveryTableManager.h"
#import "MWMDiscoveryTapDelegate.h"
#import "MWMDiscoveryCollectionView.h"
#import "MWMDiscoveryControllerViewModel.h"
#import "MWMDiscoveryHotelViewModel.h"
#import "MWMDiscoverySearchViewModel.h"
#import "MWMNetworkPolicy+UI.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#import <CoreApi/CatalogPromoItem+Core.h>

#include "map/place_page_info.hpp"

#include "search/result.hpp"

using namespace std;

namespace discovery
{
NSString * StatProvider(ItemType const type)
{
  switch (type) {
    case ItemType::Attractions: return kStatSearchAttractions;
    case ItemType::Cafes: return kStatSearchRestaurants;
    case ItemType::Hotels: return kStatBooking;
    case ItemType::Promo: return kStatMapsmeGuides;
    case ItemType::LocalExperts: return @"";
  }
}
}  // namespace discovery

using namespace discovery;

@interface MWMDiscoveryTableManager ()<UITableViewDataSource, UICollectionViewDelegate,
                                       UICollectionViewDataSource>
{
  vector<ItemType> m_types;
  vector<ItemType> m_loadingTypes;
  vector<ItemType> m_failedTypes;
}

@property(weak, nonatomic) UITableView * tableView;
@property(nonatomic) MWMGetModelCallback model;
@property(weak, nonatomic) id<MWMDiscoveryTapDelegate> delegate;
@property(nonatomic) BOOL canUseNetwork;

@end

@implementation MWMDiscoveryTableManager

#pragma mark - Public

- (instancetype)initWithTableView:(UITableView *)tableView
                    canUseNetwork:(BOOL)canUseNetwork
                         delegate:(id<MWMDiscoveryTapDelegate>)delegate
                            model:(MWMGetModelCallback)modelCallback {
  self = [super init];
  if (self) {
    _tableView = tableView;
    _delegate = delegate;
    _model = modelCallback;
    _canUseNetwork = canUseNetwork;
    tableView.dataSource = self;
    tableView.rowHeight = UITableViewAutomaticDimension;
    tableView.estimatedRowHeight = 218;
    if (@available(iOS 11.0, *))
      tableView.insetsContentViewsToSafeArea = NO;
    [self registerCells];
  }
  return self;
}

- (MWMDiscoveryControllerViewModel *)viewModel {
  return self.model();
}

- (void)loadItems:(vector<ItemType> const &)types {
  m_types = types;
  m_loadingTypes = types;
  if (!self.canUseNetwork) {
    m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), ItemType:: Promo), m_loadingTypes.end());
    m_failedTypes.push_back(ItemType:: Promo);
    [Statistics logEvent:kStatPlacepageSponsoredError
          withParameters:@{kStatProvider: StatProvider(ItemType:: Promo),
                           kStatPlacement: kStatDiscovery}];
  }
  [self.tableView reloadData];
}

- (void)reloadGuidesIfNeeded {
  MWMConnectionType connectionType = [MWMNetworkPolicy sharedPolicy].connectionType;
  BOOL isNetworkAvailable = connectionType == MWMConnectionTypeWifi ||
                            (connectionType == MWMConnectionTypeCellular && self.canUseNetwork);
  
  if (m_failedTypes.size() != 0 && isNetworkAvailable) {
    m_failedTypes.erase(remove(m_failedTypes.begin(), m_failedTypes.end(), ItemType:: Promo), m_failedTypes.end());
    m_loadingTypes.push_back(ItemType:: Promo);
    
    NSInteger position = [self position:ItemType:: Promo];
    [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:position]
                  withRowAnimation:UITableViewRowAnimationFade];
  }
}

- (void)reloadItem:(ItemType const)type {
  if ([self.viewModel itemsCountForType:type] == 0) {
    [self removeItem:type];
    return;
  }
  

  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type), m_loadingTypes.end());
  m_failedTypes.erase(remove(m_failedTypes.begin(), m_failedTypes.end(), type), m_failedTypes.end());
  
  NSInteger position = [self position:type];
  [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:position]
                withRowAnimation:UITableViewRowAnimationFade];

  [Statistics logEvent:kStatPlacepageSponsoredShow
        withParameters:@{
          kStatProvider: StatProvider(type),
          kStatPlacement: kStatDiscovery,
          kStatState: self.hasOnlineSections ? kStatOnline : kStatOffline
        }];
}

- (void)errorAtItem:(ItemType const)type {
  CHECK(type == ItemType::Promo,
        ("Error on item with type:", static_cast<int>(type)));
  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type), m_loadingTypes.end());
  m_failedTypes.push_back(type);
  NSInteger position = [self position:type];

  [Statistics logEvent:kStatPlacepageSponsoredError
        withParameters:@{kStatProvider: StatProvider(type),
                         kStatPlacement: kStatDiscovery}];

  [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:position]
                withRowAnimation:UITableViewRowAnimationFade];
}

#pragma mark - Private

- (BOOL)hasOnlineSections {
  return find(m_types.begin(), m_types.end(), ItemType::Promo) != m_types.end();
}

- (void)removeItem:(ItemType const)type {
  NSInteger position = [self position:type];
  m_types.erase(remove(m_types.begin(), m_types.end(), type), m_types.end());
  m_failedTypes.erase(remove(m_failedTypes.begin(), m_failedTypes.end(), type), m_failedTypes.end());
  m_loadingTypes.erase(remove(m_loadingTypes.begin(), m_loadingTypes.end(), type), m_loadingTypes.end());

  auto indexSet = [NSIndexSet indexSetWithIndex:position];
  auto tv = self.tableView;
  if (m_types.empty())
    [tv reloadData];
  else
    [tv deleteSections:indexSet withRowAnimation:UITableViewRowAnimationFade];
}

- (void)registerCells {
  auto tv = self.tableView;
  [tv registerNibWithCellClass:[MWMDiscoverySpinnerCell class]];
  [tv registerNibWithCellClass:[MWMDiscoveryOnlineTemplateCell class]];
  [tv registerNibWithCellClass:[MWMDiscoverySearchCollectionHolderCell class]];
  [tv registerNibWithCellClass:[MWMDiscoveryGuideCollectionHolderCell class]];
  [tv registerNibWithCellClass:[MWMDiscoveryBookingCollectionHolderCell class]];
  [tv registerNibWithCellClass:[MWMDiscoveryNoResultsCell class]];
}

- (NSInteger)position:(ItemType const)type {
  auto const it = find(m_types.begin(), m_types.end(), type);
  if (it == m_types.end())
    CHECK(false, ("Incorrect item type:", static_cast<int>(type)));
  
  return distance(m_types.begin(), it);
}

- (MWMDiscoverySearchCollectionHolderCell *)searchCollectionHolderCell:(NSIndexPath *)indexPath {
  Class cls = [MWMDiscoverySearchCollectionHolderCell class];
  ItemType const type = m_types[indexPath.section];
  MWMDiscoverySearchCollectionHolderCell *cell = (MWMDiscoverySearchCollectionHolderCell *)
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  MWMDiscoveryCollectionView *collection = (MWMDiscoveryCollectionView *)cell.collectionView;
  switch (type) {
    case ItemType::Attractions: [cell configAttractionsCell]; break;
    case ItemType::Cafes: [cell configCafesCell]; break;
  default: NSAssert(false, @""); return nil;
  }
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = type;
  return cell;
}

- (MWMDiscoveryBookingCollectionHolderCell *)bookingCollectionHolderCell:(NSIndexPath *)indexPath {
  Class cls = [MWMDiscoveryBookingCollectionHolderCell class];
  MWMDiscoveryBookingCollectionHolderCell *cell = (MWMDiscoveryBookingCollectionHolderCell *)
      [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  MWMDiscoveryCollectionView *collection = (MWMDiscoveryCollectionView *)cell.collectionView;
  [cell config];
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = ItemType::Hotels;
  return cell;
}

- (MWMDiscoveryGuideCollectionHolderCell *)guideCollectionHolderCell:(NSIndexPath *)indexPath {
  Class cls = [MWMDiscoveryGuideCollectionHolderCell class];
  MWMDiscoveryGuideCollectionHolderCell *cell = (MWMDiscoveryGuideCollectionHolderCell *)
  [self.tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  MWMDiscoveryCollectionView *collection = (MWMDiscoveryCollectionView *)cell.collectionView;
  [cell config:L(@"gallery_pp_download_guides_title")];
  collection.delegate = self;
  collection.dataSource = self;
  collection.itemType = ItemType::Promo;
  return cell;
}

- (MWMDiscoverySpinnerCell *)spinnerCell:(NSIndexPath *)indexPath {
  Class cls = [MWMDiscoverySpinnerCell class];
  return (MWMDiscoverySpinnerCell *)[self.tableView dequeueReusableCellWithCellClass:cls
                                                                           indexPath:indexPath];
}

- (MWMDiscoveryOnlineTemplateCell *)onlineTemplateCell:(NSIndexPath *)indexPath {
  Class cls = [MWMDiscoveryOnlineTemplateCell class];
  return (MWMDiscoveryOnlineTemplateCell *)[self.tableView dequeueReusableCellWithCellClass:cls
                                                                                  indexPath:indexPath];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
  return static_cast<NSInteger>(MAX(m_types.size(), 1));
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  if (m_types.empty()) {
    return 1;
  }
  ItemType const type = m_types[section];
  switch (type) {
    case ItemType::Attractions:
    case ItemType::Cafes:
    case ItemType::Hotels:
    case ItemType::Promo:
      return 1;
    case ItemType::LocalExperts:
      return 0;
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  if (m_types.empty()) {
    Class cls = [MWMDiscoveryNoResultsCell class];
    return (MWMDiscoveryNoResultsCell *)[tableView dequeueReusableCellWithCellClass:cls
                                                                          indexPath:indexPath];
  }
  
  ItemType const type = m_types[indexPath.section];
  BOOL isFailed = find(m_failedTypes.begin(), m_failedTypes.end(), type) != m_failedTypes.end();
  BOOL isLoading = find(m_loadingTypes.begin(), m_loadingTypes.end(), type) != m_loadingTypes.end();

  switch (type) {
    case ItemType::Attractions:
    case ItemType::Cafes:
      return isLoading ? [self spinnerCell:indexPath] : [self searchCollectionHolderCell:indexPath];
    case ItemType::Hotels:
      return isLoading ? [self spinnerCell:indexPath] : [self bookingCollectionHolderCell:indexPath];
    case ItemType::Promo: {
      if (isLoading || isFailed) {
        MWMDiscoveryOnlineTemplateCell *cell = [self onlineTemplateCell:indexPath];
        __weak __typeof__(self) weakSelf = self;
        [cell configWithType:MWMDiscoveryOnlineTemplateTypePromo
                 needSpinner:isLoading
               canUseNetwork: self.canUseNetwork
                         tap:^{
                           __strong __typeof__(weakSelf) strongSelf = weakSelf;
                           if (![MWMFrameworkHelper isNetworkConnected]) {
                             NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
                             UIApplication * app = UIApplication.sharedApplication;
                             if ([app canOpenURL:url])
                               [app openURL:url options:@{} completionHandler:nil];
                           } else {
                             [[MWMNetworkPolicy sharedPolicy] callOnlineApi:^(BOOL) {
                               [strongSelf reloadGuidesIfNeeded];
                             } forceAskPermission:YES];
                           }
                         }];
        return cell;
      }
      return [self guideCollectionHolderCell: indexPath];
    }
    case ItemType::LocalExperts:
      return [[UITableViewCell alloc] init];
  }
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
  NSInteger count = [self.viewModel itemsCountForType:collectionView.itemType];
  return count > 0 ? count + 1 : 0;
}

- (UICollectionViewCell *)collectionView:(MWMDiscoveryCollectionView *)collectionView
                  cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  ItemType const type = collectionView.itemType;
  
  if (indexPath.row == [self.viewModel itemsCountForType:type]) {
    Class cls = [MWMDiscoveryMoreCell class];
    MWMDiscoveryMoreCell *cell = (MWMDiscoveryMoreCell *)[collectionView
                                                          dequeueReusableCellWithCellClass:cls
                                                          indexPath:indexPath];
    return cell;
  }
  
  switch (type) {
    case ItemType::Attractions:
    case ItemType::Cafes: {
      Class cls = [MWMDiscoverySearchCell class];
      MWMDiscoverySearchCell *cell = (MWMDiscoverySearchCell *)
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
      MWMDiscoverySearchViewModel *objectVM = type == ItemType::Attractions ? [self.viewModel attractionAtIndex:indexPath.item] : [self.viewModel cafeAtIndex:indexPath.item];
      __weak __typeof__(self) weakSelf = self;
      [cell configWithTitle:objectVM.title
                   subtitle:objectVM.subtitle
                   distance:objectVM.distance
                    popular:objectVM.isPopular
                ratingValue:objectVM.ratingValue
                 ratingType:objectVM.ratingType
                        tap:^{
                          [weakSelf.delegate routeToItem:type atIndex:indexPath.row];
                        }];
      return cell;
    }
    case ItemType::Hotels: {
      Class cls = [MWMDiscoveryBookingCell class];
      MWMDiscoveryBookingCell *cell = (MWMDiscoveryBookingCell *)
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
      MWMDiscoveryHotelViewModel *objectVM = [self.viewModel hotelAtIndex:indexPath.item];
      __weak __typeof__(self) weakSelf = self;
      [cell configWithAvatarURL:nil
                          title:objectVM.title
                       subtitle:objectVM.subtitle
                          price:objectVM.price
                    ratingValue:objectVM.ratingValue
                     ratingType:objectVM.ratingType
                       distance:objectVM.distance
                   onBuildRoute:^{
                     [weakSelf.delegate routeToItem:type atIndex:indexPath.row];
                   }];
      return cell;
    }
    case ItemType::Promo: {
      Class cls = [MWMDiscoveryGuideCell class];
      MWMDiscoveryGuideCell *cell = (MWMDiscoveryGuideCell *)
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
      CatalogPromoItem *objectVM = [self.viewModel guideAtIndex:indexPath.item];
      __weak __typeof__(self) weakSelf = self;
      [cell configWithAvatarURL:objectVM.imageUrl
                          title:objectVM.guideName
                       subtitle:objectVM.guideAuthor
                          label:objectVM.categoryLabel
                  labelHexColor:objectVM.hexColor
                      onDetails:^{
                            [weakSelf.delegate openURLForItem:ItemType::Promo atIndex:indexPath.row];
                          }];
      return cell;
    }
    case ItemType::LocalExperts:
      return [[UICollectionViewCell alloc] init];
  }
}

@end
