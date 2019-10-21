#pragma once

#include "map/discovery/discovery_client_params.hpp"
#include "map/discovery/discovery_search.hpp"
#include "map/discovery/discovery_search_params.hpp"
#include "map/search_api.hpp"
#include "map/search_product_info.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/locals_api.hpp"
#include "partners_api/promo_api.hpp"

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
    APIs(SearchAPI & search, promo::Api & promo, locals::Api & locals)
      : m_search(search), m_promo(promo), m_locals(locals)
    {
    }

    SearchAPI & m_search;
    promo::Api & m_promo;
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

  Manager(DataSource const & dataSource, APIs const & apis);

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
        auto const latLon = mercator::ToLatLon(params.m_viewportCenter);
        auto constexpr pageNumber = 1;
        m_localsApi.GetLocals(
            latLon.m_lat, latLon.m_lon, params.m_lang, params.m_itemsCount, pageNumber,
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
      case ItemType::Promo:
      {
        m_promoApi.GetCityGallery(
            params.m_viewportCenter, params.m_lang, UTM::DiscoveryPageGallery,
            [this, requestId, onResult](promo::CityGallery const & cityGallery) {
              CHECK_THREAD_CHECKER(m_threadChecker, ());
              onResult(requestId, cityGallery);
            },
            [this, requestId, onError, type]() {
              CHECK_THREAD_CHECKER(m_threadChecker, ());
              onError(requestId, type);
            });
        break;
      }
      }
    }
    return requestId;
  }

  std::string GetLocalExpertsUrl(m2::PointD const & point) const;

private:
  static DiscoverySearchParams GetSearchParams(Manager::Params const & params, ItemType const type);

  DataSource const & m_dataSource;
  SearchAPI & m_searchApi;
  promo::Api & m_promoApi;
  locals::Api & m_localsApi;
  uint32_t m_requestCounter = 0;
  ThreadChecker m_threadChecker;
};
}  // namespace discovery
