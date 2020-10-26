#import "MWMSearchFrameworkHelper.h"
#import "CoreBanner+Core.h"

#include "partners_api/ads/ads_engine.hpp"
#include "partners_api/megafon_countries.hpp"

#include "platform/preferred_languages.hpp"

#include "Framework.h"

@implementation MWMSearchFrameworkHelper

- (NSArray<NSString *> *)searchCategories
{
  NSMutableArray * result = [NSMutableArray array];
  auto const & categories = GetFramework().GetDisplayedCategories().GetKeys();
  for (auto const & item : categories)
  {
    [result addObject:@(item.c_str())];
  }
  return [result copy];
}

- (id<MWMBanner>)searchCategoryBanner
{
  if (GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE)
    return nil;

  auto const & f = GetFramework();
  auto const pos = f.GetCurrentPosition();
  auto const banners = f.GetAdsEngine().GetSearchCategoryBanners(pos);

  if (banners.empty())
    return nil;

  return [[CoreBanner alloc] initWithAdBanner:banners.front()];
}

- (BOOL)isSearchHistoryEmpty
{
  return GetFramework().GetSearchAPI().GetLastSearchQueries().empty();
}

- (NSArray<NSString *> *)lastSearchQueries
{
  NSMutableArray * result = [NSMutableArray array];
  auto const & queries = GetFramework().GetSearchAPI().GetLastSearchQueries();
  for (auto const & item : queries)
  {
    [result addObject:@(item.second.c_str())];
  }
  return [result copy];
}

- (void)clearSearchHistory
{
  GetFramework().GetSearchAPI().ClearSearchHistory();
}

@end
