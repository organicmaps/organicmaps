#import "MWMDiscoveryController.h"

#import <CoreApi/CoreApi.h>

#import "MWMDiscoveryControllerViewModel.h"
#import "MWMDiscoveryCityGalleryObjects.h"
#import "MWMDiscoveryMapObjects.h"
#import "MWMDiscoveryTableManager.h"
#import "MWMDiscoveryTapDelegate.h"
#import "MWMMapViewControlsManager.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearchManager+Filter.h"
#import "SwiftBridge.h"

#include "platform/localization.hpp"

using namespace std;
using namespace std::placeholders;
using namespace discovery;

namespace
{
struct Callback
{
  void operator()(uint32_t const requestId, search::Results const & results,
                  vector<search::ProductInfo> const & productInfo, ItemType const type,
                  m2::PointD const & viewportCenter) const
  {
    CHECK(m_setSearchResults, ());
    CHECK(m_refreshSection, ());
    m_setSearchResults(results, productInfo, viewportCenter, type);
    m_refreshSection(type);
  }
  
  void operator()(uint32_t const requestId, vector<locals::LocalExpert> const & experts) const
  {
    // TODO: Please add correct implementation here.
  }
  
  void operator()(uint32_t const requestId, promo::CityGallery const & experts) const
  {
    CHECK(m_setPromoCityGallery, ());
    CHECK(m_refreshSection, ());
    m_setPromoCityGallery(experts);
    m_refreshSection(ItemType::Promo);
  }

  using SetSearchResults =
      function<void(search::Results const & res, vector<search::ProductInfo> const & productInfo,
                    m2::PointD const & viewportCenter, ItemType const type)>;
  using SetPromoCityGallery = function<void(promo::CityGallery const & experts)>;
  using RefreshSection = function<void(ItemType const type)>;

  SetSearchResults m_setSearchResults;
  SetPromoCityGallery m_setPromoCityGallery;
  RefreshSection m_refreshSection;
};
}  // namespace

@interface MWMDiscoveryController ()<MWMDiscoveryTapDelegate>
{
  Callback m_callback;
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(nonatomic) MWMDiscoveryTableManager * tableManager;
@property(nonatomic) MWMDiscoveryControllerViewModel *viewModel;
@property(nonatomic) BOOL canUseNetwork;

@end

@implementation MWMDiscoveryController

+ (instancetype)instanceWithConnection:(BOOL)canUseNetwork {
  auto instance = [[MWMDiscoveryController alloc] initWithNibName:self.className bundle:nil];
  instance.title = L(@"discovery_button_title");
  instance.canUseNetwork = canUseNetwork;
  return instance;
}

- (instancetype)initWithNibName:(NSString *)name bundle:(NSBundle *)bundle {
  self = [super initWithNibName:name bundle:bundle];
  if (self) {
    _viewModel = [[MWMDiscoveryControllerViewModel alloc] init];
    auto & cb = m_callback;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-repeated-use-of-weak"
     __weak __typeof__(self) weakSelf = self;
    cb.m_setSearchResults = [weakSelf](search::Results const & res, vector<search::ProductInfo> const & productInfo, m2::PointD const & viewportCenter, ItemType const type) {
      __strong __typeof__(weakSelf) strongSelf = weakSelf;
      if (!strongSelf) { return; }
      MWMDiscoveryMapObjects *objects = [[MWMDiscoveryMapObjects alloc] initWithSearchResults:res
                                                                                 productInfos:productInfo
                                                                               viewPortCenter:viewportCenter];
      [strongSelf.viewModel updateMapObjects:objects forType:type];
    };
    cb.m_setPromoCityGallery = [weakSelf](promo::CityGallery const & experts) {
      __strong __typeof__(weakSelf) strongSelf = weakSelf;
      if (!strongSelf) { return; }
      MWMDiscoveryCityGalleryObjects *objects = [[MWMDiscoveryCityGalleryObjects alloc] initWithGalleryResults:experts];
      [strongSelf.viewModel updateCityGalleryObjects:objects];
    };
    cb.m_refreshSection = [weakSelf](ItemType const type) {
      __strong __typeof__(weakSelf) strongSelf = weakSelf;
      if (!strongSelf) { return; }
      [strongSelf.tableManager reloadItem:type];
    };
#pragma clang diagnostic pop
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  __weak __typeof__(self) weakSelf = self;
  MWMGetModelCallback callback = ^{
    return weakSelf.viewModel;
  };
  self.tableManager = [[MWMDiscoveryTableManager alloc] initWithTableView:self.tableView
                                                            canUseNetwork:self.canUseNetwork
                                                                 delegate:self
                                                                    model:callback];
  vector<ItemType> types = {ItemType::Promo, ItemType::Attractions, ItemType::Cafes, ItemType::Hotels};
  [self.tableManager loadItems:types];
  ClientParams p;
  p.m_itemTypes = move(types);
  GetFramework().Discover(move(p), m_callback,
                          [self](uint32_t const requestId, ItemType const type) {
                            [self.tableManager errorAtItem:type];
                          });
  
  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(didBecomeActive)
                                             name:UIApplicationDidBecomeActiveNotification
                                           object:nil];
}

#pragma mark - MWMDiscoveryTapDelegate

- (void)showSearchResult:(const search::Result &)item {
  GetFramework().ShowSearchResult(item);
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)tapOnItem:(ItemType const)type atIndex:(NSInteger)index {
  switch (type) {
    case ItemType::Attractions:
      if (index == [self.viewModel itemsCountForType:type]) {
        [self searchTourism];
      } else {
        [self showSearchResult:[self.viewModel.attractions searchResultAtIndex:index]];
      }
      break;
    case ItemType::Cafes:
      if (index == [self.viewModel itemsCountForType:type]) {
        [self searchFood];
      } else {
        [self showSearchResult:[self.viewModel.cafes searchResultAtIndex:index]];
      }
      break;
    case ItemType::Hotels:
      if (index == [self.viewModel itemsCountForType:type]) {
        [self openFilters];
      } else {
        [self showSearchResult:[self.viewModel.hotels searchResultAtIndex:index]];
      }
      break;
    case ItemType::Promo:
      if (index == [self.viewModel itemsCountForType:type]) {
        [self openURLForItem:ItemType::Promo];
      } else {
        [self openURLForItem:ItemType::Promo atIndex:index];
      }
      
      return;
    case ItemType::LocalExperts:
      return;
  }
}

- (void)openFilters {
  [self.navigationController popViewControllerAnimated:YES];
  [[MWMSearchManager manager] showHotelFilterWithParams:nil
                                       onFinishCallback:^{
                                         [MWMMapViewControlsManager.manager
                                          searchTextOnMap:[L(@"hotel") stringByAppendingString:@" "]
                                          forInputLocale:[NSLocale currentLocale].localeIdentifier];
                                       }];
}

- (void)searchFood {
  [self.navigationController popViewControllerAnimated:YES];
  [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"eat") stringByAppendingString:@" "]
                                      forInputLocale:[NSLocale currentLocale].localeIdentifier];
}

- (void)searchTourism {
  [self.navigationController popViewControllerAnimated:YES];
  [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"tourism") stringByAppendingString:@" "]
                                      forInputLocale:[NSLocale currentLocale].localeIdentifier];
}

- (void)routeToItem:(ItemType const)type atIndex:(NSInteger)index {
  __block m2::PointD point;
  __block NSString * title;
  __block NSString * subtitle;

  auto getRoutePointInfo = ^(search::Result const & item) {
    point = item.GetFeatureCenter();

    ASSERT(item.GetResultType() == search::Result::Type::Feature, ());
    auto const readableType = classif().GetReadableObjectName(item.GetFeatureType());

    subtitle = @(platform::GetLocalizedTypeName(readableType).c_str());
    title = item.GetString().empty() ? subtitle : @(item.GetString().c_str());
  };

  switch (type) {
    case ItemType::Attractions:
      getRoutePointInfo([self.viewModel.attractions searchResultAtIndex:index]);
      break;
    case ItemType::Cafes:
      getRoutePointInfo([self.viewModel.cafes searchResultAtIndex:index]);
      break;
    case ItemType::Hotels:
      getRoutePointInfo([self.viewModel.hotels searchResultAtIndex:index]);
      break;
    case ItemType::Promo:
    case ItemType::LocalExperts:
      return;
  }

  MWMRoutePoint * pt = [[MWMRoutePoint alloc] initWithPoint:point
                                                      title:title
                                                   subtitle:subtitle
                                                       type:MWMRoutePointTypeFinish
                                          intermediateIndex:0];
  [MWMRouter setType:MWMRouterTypePedestrian];
  [MWMRouter buildToPoint:pt bestRouter:NO];
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)openURLForItem:(ItemType const)type atIndex:(NSInteger)index {
  switch (type) {
    case ItemType::Attractions:
    case ItemType::Cafes:
    case ItemType::Hotels:
    case ItemType::LocalExperts:
      break;
    case ItemType::Promo:
      promo::CityGallery::Item const &item = [self.viewModel.guides galleryItemAtIndex:index];
      NSString *itemPath = @(item.m_url.c_str());
      if (!itemPath || itemPath.length == 0) {
        return;
      }
      NSURL *url = [NSURL URLWithString:itemPath];
      [self openCatalogForURL:url];
      break;
  }
}

- (void)openURLForItem:(ItemType const)type {
  switch (type) {
    case ItemType::Attractions:
    case ItemType::Cafes:
    case ItemType::Hotels:
    case ItemType::LocalExperts:
      break;
    case ItemType::Promo:
      NSURL *url = [self.viewModel.guides moreURL];
      [self openCatalogForURL:url];
      break;
  }
}

- (void)openCatalogForURL:(NSURL *)url {
  // NOTE: UTM is already into URL, core part does it for Discovery page.
  MWMCatalogWebViewController *catalog = [MWMCatalogWebViewController catalogFromAbsoluteUrl:url utm:MWMUTMNone];
  NSMutableArray<UIViewController *> * controllers = [self.navigationController.viewControllers mutableCopy];
  [controllers addObjectsFromArray:@[catalog]];
  [self.navigationController setViewControllers:controllers animated:YES];
}

- (void)didBecomeActive
{
  [self.tableManager reloadGuidesIfNeeded];
}

@end
