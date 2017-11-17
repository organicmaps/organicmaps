#pragma once

#include "search/city_finder.hpp"

#include "map/discovery_client_params.hpp"
#include "map/search_api.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <string>
#include <vector>

namespace discovery
{
class Manager final
{
public:
  struct APIs
  {
    APIs(SearchAPI * const search, viator::Api * const viator, locals::Api * const locals)
      : m_search(search), m_viator(viator), m_locals(locals)
    {
    }
    SearchAPI * const m_search;
    viator::Api * const m_viator;
    locals::Api * const m_locals;
  };

  Manager(Index const & index, search::CityFinder * const cityFinder, APIs const & apis);

  struct Params
  {
    std::string m_curency;
    std::string m_lang;
    size_t m_itemsCount;
    m2::PointD m_viewportCenter;
    m2::RectD m_viewport;
    ItemTypes m_itemTypes;
  };

  using ErrorCalback = std::function<void(ItemType const type)>;

  template <typename ResultCallback>
  void Discover(Params && params, ResultCallback const & result, ErrorCalback const & error)
  {
    auto const & types = params.m_itemTypes;
    ASSERT(!types.empty(), ("Types must contain at least one element."));

    for (auto const type : types)
    {
      switch (type)
      {
      case ItemType::Viator:
      {
        string const sponsoredId = GetCityViatorId(params.m_viewportCenter);
        if (sponsoredId.empty())
        {
          error(type);
          break;
        }

        m_viatorApi->GetTop5Products(
            sponsoredId, params.m_curency,
            [sponsoredId, result, type](string const & destId,
                                        std::vector<viator::Product> const & products) {
              if (destId == sponsoredId)
                result(products, type);
            });
        break;
      }
      case ItemType::Attractions:
      case ItemType::Cafe:
      {
        auto p = GetSearchParams(params, type);
        p.m_onResults = [result, type](search::Results const & results) {
          if (results.IsEndMarker())
            result(results, type);
        };
        m_searchApi->GetEngine().Search(p);
        break;
      }
      case ItemType::Hotels:
      {
        ASSERT(false, ("Discovering hotels isn't supported yet."));
        break;
      }
      case ItemType::LocalExperts:
      {
        auto const latLon = MercatorBounds::ToLatLon(params.m_viewportCenter);
        auto constexpr pageNumber = 1;
        m_localsApi->GetLocals(
            latLon.lat, latLon.lon, params.m_lang, params.m_itemsCount, pageNumber,
            [result, type](uint64_t id, std::vector<locals::LocalExpert> const & locals,
                           size_t /* pageNumber */, size_t /* countPerPage */,
                           bool /* hasPreviousPage */,
                           bool /* hasNextPage */) { result(locals, type); },
            [error, type](uint64_t id, int errorCode, std::string const & errorMessage) {
              error(type);
            });
        break;
      }
      }
    }
  }

private:
  static search::SearchParams GetSearchParams(Params const & params, ItemType const type);
  std::string GetCityViatorId(m2::PointD const & point) const;

private:
  Index const & m_index;
  search::CityFinder * const m_cityFinder;
  SearchAPI * const m_searchApi;
  viator::Api * const m_viatorApi;
  locals::Api * const m_localsApi;
};
}  // namespace discovery
