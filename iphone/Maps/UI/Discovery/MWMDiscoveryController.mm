#import "MWMDiscoveryController.h"
#import "Framework.h"
#import "MWMDiscoveryTableManager.h"
#import "MWMDiscoveryTapDelegate.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "Statistics.h"
#import "UIKitCategories.h"

#include "DiscoveryControllerViewModel.hpp"

#include "map/discovery/discovery_client_params.hpp"

#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "search/result.hpp"

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
  void operator()(uint32_t const requestId, search::Results const & results, ItemType const type,
                  m2::PointD const & viewportCenter) const
  {
    CHECK(m_setSearchResults, ());
    CHECK(m_refreshSection, ());
    m_setSearchResults(results, viewportCenter, type);
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

  using SetSearchResults = function<void(search::Results const & res,
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
@property(nonatomic) MWMDiscoveryMode mode;

@end

@implementation MWMDiscoveryController

+ (instancetype)instance
{
  auto instance = [[MWMDiscoveryController alloc] initWithNibName:self.className bundle:nil];
  instance.title = L(@"discovery_button_title");
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
        bind(&DiscoveryControllerViewModel::SetSearchResults, &m_model, _1, _2, _3);
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

  auto getTypes = [](MWMDiscoveryMode m) -> vector<ItemType> {
    if (m == MWMDiscoveryModeOnline)
      return {ItemType::Viator, ItemType::Attractions, ItemType::Cafes, ItemType::LocalExperts};
    return {ItemType::Attractions, ItemType::Cafes};
  };

  vector<ItemType> types = getTypes(self.mode);
  [self.tableManager loadItems:types];
  ClientParams p;
  p.m_itemTypes = move(types);
  GetFramework().Discover(move(p), m_callback,
                          [self](uint32_t const requestId, ItemType const type) {
                            [self.tableManager errorAtItem:type];
                          });
}

#pragma mark - MWMDiscoveryTapDelegate

- (void)tapOnItem:(ItemType const)type atIndex:(size_t const)index
{
  NSString * dest = @"";
  switch (type)
  {
  case ItemType::Viator:
  case ItemType::LocalExperts:
  {
    auto const & url = type == ItemType::Viator ? m_model.GetViatorAt(index).m_pageUrl
                                                : m_model.GetExpertAt(index).m_pageUrl;
    [self openUrl:[NSURL URLWithString:@(url.c_str())]];
    dest = kStatExternal;
    break;
  }
  case ItemType::Attractions:
  case ItemType::Cafes:
  {
    auto const & item =
        type == ItemType::Attractions ? m_model.GetAttractionAt(index) : m_model.GetCafeAt(index);
    GetFramework().ShowSearchResult(item);
    [self.navigationController popViewControllerAnimated:YES];
    dest = kStatPlacePage;
    break;
  }
  case ItemType::Hotels:
  {
    NSAssert(false, @"Discovering hotels hasn't implemented yet.");
    break;
  }
  }

  NSAssert(dest.length > 0, @"");
  [Statistics logEvent:kStatDiscoveryButtonItemClick
        withParameters:@{
          kStatProvider: StatProvider(type),
          kStatPlacement: kStatDiscovery,
          kStatItem: @(index + 1),
          kStatDestination: dest
        }];
}

- (void)routeToItem:(ItemType const)type atIndex:(size_t const)index
{
  CHECK(type == ItemType::Attractions || type == ItemType::Cafes,
        ("Attempt to route to item with type:", static_cast<int>(type)));
  auto const & item =
      type == ItemType::Attractions ? m_model.GetAttractionAt(index) : m_model.GetCafeAt(index);
  MWMRoutePoint * pt = [[MWMRoutePoint alloc] initWithPoint:item.GetFeatureCenter()
                                                      title:@(item.GetString().c_str())
                                                   subtitle:@(item.GetFeatureTypeName().c_str())
                                                       type:MWMRoutePointTypeFinish
                                          intermediateIndex:0];
  [MWMRouter setType:MWMRouterTypePedestrian];
  [MWMRouter buildToPoint:pt bestRouter:NO];
  [self.navigationController popViewControllerAnimated:YES];

  [Statistics logEvent:kStatDiscoveryButtonItemClick
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
