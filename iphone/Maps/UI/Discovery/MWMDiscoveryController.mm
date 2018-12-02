#import "MWMDiscoveryController.h"

#import "Framework.h"
#import "MWMDiscoveryTableManager.h"
#import "MWMDiscoveryTapDelegate.h"
#import "MWMEye.h"
#import "MWMMapViewControlsManager.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearch.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "Statistics.h"
#import "UIKitCategories.h"

#include "DiscoveryControllerViewModel.hpp"

#include "map/discovery/discovery_client_params.hpp"
#include "map/search_product_info.hpp"

#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "search/result.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <functional>
#include <utility>
#include <vector>

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

  void operator()(uint32_t const requestId, vector<viator::Product> const & products) const
  {
    CHECK(m_setViatorProducts, ());
    CHECK(m_refreshSection, ());
    m_setViatorProducts(products);
    m_refreshSection(ItemType::Viator);
  }

  void operator()(uint32_t const requestId, vector<locals::LocalExpert> const & experts) const
  {
    CHECK(m_setLocalExperts, ());
    CHECK(m_refreshSection, ());
    m_setLocalExperts(experts);
    m_refreshSection(ItemType::LocalExperts);
  }

  using SetSearchResults =
      function<void(search::Results const & res, vector<search::ProductInfo> const & productInfo,
                    m2::PointD const & viewportCenter, ItemType const type)>;
  using SetViatorProducts = function<void(vector<viator::Product> const & viator)>;
  using SetLocalExperts = function<void(vector<locals::LocalExpert> const & experts)>;
  using RefreshSection = function<void(ItemType const type)>;

  SetSearchResults m_setSearchResults;
  SetViatorProducts m_setViatorProducts;
  SetLocalExperts m_setLocalExperts;
  RefreshSection m_refreshSection;
};
}  // namespace

@interface MWMDiscoveryController ()<MWMDiscoveryTapDelegate>
{
  Callback m_callback;
  DiscoveryControllerViewModel m_model;
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(nonatomic) MWMDiscoveryTableManager * tableManager;
@property(nonatomic) BOOL canUseNetwork;

@end

@implementation MWMDiscoveryController

+ (instancetype)instanceWithConnection:(BOOL)canUseNetwork
{
  auto instance = [[MWMDiscoveryController alloc] initWithNibName:self.className bundle:nil];
  instance.title = L(@"discovery_button_title");
  instance.canUseNetwork = canUseNetwork;
  return instance;
}

- (instancetype)initWithNibName:(NSString *)name bundle:(NSBundle *)bundle
{
  self = [super initWithNibName:name bundle:bundle];
  if (self)
  {
    auto & cb = m_callback;
    cb.m_setLocalExperts = bind(&DiscoveryControllerViewModel::SetExperts, &m_model, _1);
    cb.m_setSearchResults =
        bind(&DiscoveryControllerViewModel::SetSearchResults, &m_model, _1, _2, _3, _4);
    cb.m_setViatorProducts = bind(&DiscoveryControllerViewModel::SetViator, &m_model, _1);
    cb.m_refreshSection = [self](ItemType const type) { [self.tableManager reloadItem:type]; };
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  auto callback = [self]() -> DiscoveryControllerViewModel const & { return self->m_model; };
  self.tableManager = [[MWMDiscoveryTableManager alloc] initWithTableView:self.tableView
                                                                 delegate:self
                                                                    model:move(callback)];

  auto getTypes = [](BOOL canUseNetwork) -> vector<ItemType> {
    if (canUseNetwork)
      return {ItemType::Hotels, ItemType::Attractions, ItemType::Cafes, ItemType::LocalExperts};
    return {ItemType::Hotels, ItemType::Attractions, ItemType::Cafes};
  };

  vector<ItemType> types = getTypes(self.canUseNetwork);
  [self.tableManager loadItems:types];
  ClientParams p;
  p.m_itemTypes = move(types);
  GetFramework().Discover(move(p), m_callback,
                          [self](uint32_t const requestId, ItemType const type) {
                            [self.tableManager errorAtItem:type];
                          });
  [MWMEye discoveryShown];
}

#pragma mark - MWMDiscoveryTapDelegate

- (void)showSearchResult:(const search::Result &)item
{
  GetFramework().ShowSearchResult(item);
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)tapOnItem:(ItemType const)type atIndex:(size_t const)index
{
  NSString * dest = @"";
  NSString * event = kStatPlacepageSponsoredItemSelected;
  MWMEyeDiscoveryEvent eyeEvent;
  switch (type)
  {
  case ItemType::Viator:
    [self openUrl:[NSURL URLWithString:@(m_model.GetViatorAt(index).m_pageUrl.c_str())]];
    dest = kStatExternal;
    CHECK(false, ("Not reachable"));
    return;
    break;
  case ItemType::LocalExperts:
    if (index == m_model.GetItemsCount(type))
    {
      [self openURLForItem:type];
      event = kStatPlacepageSponsoredMoreSelected;
      eyeEvent = MWMEyeDiscoveryEventMoreLocals;
    }
    else
    {
      [self openUrl:[NSURL URLWithString:@(m_model.GetExpertAt(index).m_pageUrl.c_str())]];
      eyeEvent = MWMEyeDiscoveryEventLocals;
    }
    dest = kStatExternal;
    break;
  case ItemType::Attractions:
    if (index == m_model.GetItemsCount(type))
    {
      [self searchTourism];
      eyeEvent = MWMEyeDiscoveryEventMoreAttractions;
    }
    else
    {
      [self showSearchResult:m_model.GetAttractionAt(index)];
      eyeEvent = MWMEyeDiscoveryEventAttractions;
    }

    dest = kStatPlacePage;
    break;
  case ItemType::Cafes:
    if (index == m_model.GetItemsCount(type))
    {
      [self searchFood];
      eyeEvent = MWMEyeDiscoveryEventMoreCafes;
    }
    else
    {
      [self showSearchResult:m_model.GetCafeAt(index)];
      eyeEvent = MWMEyeDiscoveryEventCafes;
    }

    dest = kStatPlacePage;
    break;
  case ItemType::Hotels:
    if (index == m_model.GetItemsCount(type))
    {
      [self openFilters];
      event = kStatPlacepageSponsoredMoreSelected;
      dest = kStatSearchFilterOpen;
      eyeEvent = MWMEyeDiscoveryEventMoreHotels;
    }
    else
    {
      [self showSearchResult:m_model.GetHotelAt(index)];
      dest = kStatPlacePage;
      eyeEvent = MWMEyeDiscoveryEventHotels;
    }
    break;
  }

  NSAssert(dest.length > 0, @"");
  [Statistics logEvent:kStatPlacepageSponsoredItemSelected
        withParameters:@{
          kStatProvider: StatProvider(type),
          kStatPlacement: kStatDiscovery,
          kStatItem: @(index + 1),
          kStatDestination: dest
        }];
  [MWMEye discoveryItemClickedWithEvent:eyeEvent];
}

- (void)openFilters
{
  [self.navigationController popViewControllerAnimated:YES];
  [MWMSearch showHotelFilterWithParams:{}
                      onFinishCallback:^{
                        [MWMMapViewControlsManager.manager
                         searchTextOnMap:[L(@"hotel") stringByAppendingString:@" "]
                         forInputLocale:[NSLocale currentLocale].localeIdentifier];
                      }];
}

- (void)searchFood
{
  [self.navigationController popViewControllerAnimated:YES];
  [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"eat") stringByAppendingString:@" "]
                                      forInputLocale:[NSLocale currentLocale].localeIdentifier];
}

- (void)searchTourism
{
  [self.navigationController popViewControllerAnimated:YES];
  [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"tourism") stringByAppendingString:@" "]
                                      forInputLocale:[NSLocale currentLocale].localeIdentifier];
}

- (void)routeToItem:(ItemType const)type atIndex:(size_t const)index
{
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

  switch (type)
  {
  case ItemType::Attractions: getRoutePointInfo(m_model.GetAttractionAt(index)); break;
  case ItemType::Cafes: getRoutePointInfo(m_model.GetCafeAt(index)); break;
  case ItemType::Hotels: getRoutePointInfo(m_model.GetHotelAt(index)); break;
  case ItemType::Viator:
  case ItemType::LocalExperts:
    CHECK(false, ("Attempt to route to item with type:", static_cast<int>(type)));
    break;
  }

  MWMRoutePoint * pt = [[MWMRoutePoint alloc] initWithPoint:point
                                                      title:title
                                                   subtitle:subtitle
                                                       type:MWMRoutePointTypeFinish
                                          intermediateIndex:0];
  [MWMRouter setType:MWMRouterTypePedestrian];
  [MWMRouter buildToPoint:pt bestRouter:NO];
  [self.navigationController popViewControllerAnimated:YES];

  [Statistics logEvent:kStatPlacepageSponsoredItemSelected
        withParameters:@{
          kStatProvider: StatProvider(type),
          kStatPlacement: kStatDiscovery,
          kStatItem: @(index + 1),
          kStatDestination: kStatRouting
        }];
}

- (void)openURLForItem:(discovery::ItemType const)type
{
  CHECK(type == ItemType::Viator || type == ItemType::LocalExperts,
        ("Attempt to open url for item with type:", static_cast<int>(type)));
  auto & f = GetFramework();
  auto const url =
      type == ItemType::Viator ? f.GetDiscoveryViatorUrl() : f.GetDiscoveryLocalExpertsUrl();

  [self openUrl:[NSURL URLWithString:@(url.c_str())]];
}

- (void)tapOnLogo:(discovery::ItemType const)type
{
  CHECK(type == ItemType::Viator,
        ("Attempt to open url for item with type:", static_cast<int>(type)));
  [Statistics logEvent:kStatPlacepageSponsoredLogoSelected
        withParameters:@{kStatProvider: StatProvider(type), kStatPlacement: kStatDiscovery}];
  [self openURLForItem:type];
}

@end
