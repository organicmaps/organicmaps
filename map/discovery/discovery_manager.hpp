#pragma once

#include "search/city_finder.hpp"

#include "map/discovery/discovery_client_params.hpp"
#include "map/discovery/discovery_search.hpp"
#include "map/discovery/discovery_search_params.hpp"
#include "map/search_api.hpp"
#include "map/search_product_info.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "platform/marketing_service.hpp"
#include "platform/platform.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/thread_checker.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class DataSource;

namespace discovery
{
class Manager final
{
public:
  struct APIs
  {
    APIs(SearchAPI & search, viator::Api const & viator, locals::Api & locals)
      : m_search(search), m_viator(viator), m_locals(locals)
    {
    }

    SearchAPI & m_search;
    viator::Api const & m_viator;
    locals::Api & m_locals;
  };

  struct Params
  {
    std::string m_curency;
    std::string m_lang;
    size_t m_itemsCount = 0;
    m2::PointD m_viewportCenter;
    m2::RectD m_viewport;
    ItemTypes m_itemTypes;
  };

  using ErrorCalback = std::function<void(uint32_t const requestId, ItemType const type)>;

  Manager(DataSource const & dataSource, search::CityFinder & cityFinder, APIs const & apis);

  template <typename ResultCallback>
  uint32_t Discover(Params && params, ResultCallback const & onResult, ErrorCalback const & onError)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kDiscoveryButtonDiscovered);

    uint32_t const requestId = ++m_requestCounter;
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    auto const & types = params.m_itemTypes;
    ASSERT(!types.empty(), ("Types must contain at least one element."));

    for (auto const type : types)
    {
      switch (type)
      {
      case ItemType::Viator:
      {
        std::string const sponsoredId = GetCityViatorId(params.m_viewportCenter);
        if (sponsoredId.empty())
        {
          GetPlatform().RunTask(Platform::Thread::Gui,
                                [requestId, onResult] {
                                    onResult(requestId, std::vector<viator::Product>());
                                });
          break;
        }

        if (m_cachedViator.first == sponsoredId)
        {
          GetPlatform().RunTask(Platform::Thread::Gui, [this, requestId, onResult] {
            onResult(requestId, m_cachedViator.second);
          });
          break;
        }

        m_viatorApi.GetTop5Products(
            sponsoredId, params.m_curency,
            [this, requestId, sponsoredId, onResult, onError](std::string const & destId,
                                                     std::vector<viator::Product> const & products) {
              CHECK_THREAD_CHECKER(m_threadChecker, ());
              if (destId == sponsoredId)
              {
                if (products.empty())
                {
                  onError(requestId, ItemType::Viator);
                  return;
                }
                m_cachedViator = std::make_pair(sponsoredId, products);
                onResult(requestId, products);
              }
            });
        break;
      }
      case ItemType::Attractions: // fallthrough
      case ItemType::Cafes:
      case ItemType::Hotels:
      {
        auto p = GetSearchParams(params, type);
        auto const viewportCenter = params.m_viewportCenter;
        p.m_onResults =
          [requestId, onResult, type, viewportCenter](search::Results const & results,
                                                      std::vector<search::ProductInfo> const & productInfo) {
          GetPlatform().RunTask(Platform::Thread::Gui,
                                [requestId, onResult, type, results, productInfo, viewportCenter] {
            onResult(requestId, results, productInfo, type, viewportCenter);
          });
        };

        if (type == ItemType::Hotels)
          ProcessSearchIntent(std::make_shared<SearchHotels>(m_dataSource, p, m_searchApi));
        else
          ProcessSearchIntent(std::make_shared<SearchPopularPlaces>(m_dataSource, p, m_searchApi));

        break;
      }
      case ItemType::LocalExperts:
      {
        auto const latLon = MercatorBounds::ToLatLon(params.m_viewportCenter);
        auto constexpr pageNumber = 1;
        m_localsApi.GetLocals(
            latLon.lat, latLon.lon, params.m_lang, params.m_itemsCount, pageNumber,
            [this, requestId, onResult](uint64_t id, std::vector<locals::LocalExpert> const & locals,
                                        size_t /* pageNumber */, size_t /* countPerPage */,
                                        bool /* hasPreviousPage */, bool /* hasNextPage */) {
              CHECK_THREAD_CHECKER(m_threadChecker, ());
              onResult(requestId, locals);
            },
            [this, requestId, onError, type](uint64_t id, int errorCode, std::string const & errorMessage) {
              CHECK_THREAD_CHECKER(m_threadChecker, ());
              onError(requestId, type);
            });
        break;
      }
      }
    }
    return requestId;
  }

  std::string GetViatorUrl(m2::PointD const & point) const;
  std::string GetLocalExpertsUrl(m2::PointD const & point) const;

private:
  static DiscoverySearchParams GetSearchParams(Manager::Params const & params, ItemType const type);
  std::string GetCityViatorId(m2::PointD const & point) const;

  DataSource const & m_dataSource;
  search::CityFinder & m_cityFinder;
  SearchAPI & m_searchApi;
  viator::Api const & m_viatorApi;
  locals::Api & m_localsApi;
  uint32_t m_requestCounter = 0;
  ThreadChecker m_threadChecker;

  // We save last succeed viator result for the nearest city and rewrite it when the nearest city
  // was changed.
  std::pair<std::string, std::vector<viator::Product>> m_cachedViator;
};
}  // namespace discovery
