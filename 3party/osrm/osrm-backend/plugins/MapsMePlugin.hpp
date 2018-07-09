#pragma once

#include "../../../../base/string_utils.hpp"
#include "../../../../coding/file_container.hpp"
#include "../../../../coding/read_write_utils.hpp"
#include "../../../../defines.hpp"
#include "../../../../geometry/mercator.hpp"
#include "../../../../geometry/region2d.hpp"
#include "../../../../storage/country_decl.hpp"
#include "../../../../storage/country_polygon.hpp"
#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../data_structures/search_engine.hpp"
#include "../data_structures/edge_based_node_data.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/gpx_descriptor.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"
#include "../util/make_unique.hpp"
#include "../util/simple_logger.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

using TMapRepr = pair<size_t, m2::PointD>;

class UsedMwmChecker
{
public:
  static size_t constexpr kInvalidIndex = numeric_limits<size_t>::max();

  UsedMwmChecker() : m_lastUsedMwm(kInvalidIndex) {}

  void AddPoint(size_t mwmIndex, m2::PointD const & pt)
  {
    if (mwmIndex == kInvalidIndex)
      return;
    if (mwmIndex != m_lastUsedMwm)
    {
      CommitUsedPoints();
      m_lastUsedMwm = mwmIndex;
    }
    m_lastMwmPoints.push_back(pt);
  }

  vector<TMapRepr> const & GetUsedMwms()
  {
    // Get point from the last mwm.
    CommitUsedPoints();

    std::sort(m_usedMwms.begin(), m_usedMwms.end(), []
              (TMapRepr const & a, TMapRepr const & b)
              {
                return a.first < b.first;
              });
    auto const it = std::unique(m_usedMwms.begin(), m_usedMwms.end(), []
                                (TMapRepr const & a, TMapRepr const & b)
                                {
                                    return a.first == b.first;
                                });
    m_usedMwms.erase(it, m_usedMwms.end());
    return m_usedMwms;
  }

private:
  void CommitUsedPoints()
  {
    if (!m_lastMwmPoints.empty() && m_lastUsedMwm != kInvalidIndex)
    {
      size_t delta = m_lastMwmPoints.size() / 2;
      m_usedMwms.emplace_back(m_lastUsedMwm, m_lastMwmPoints[delta]);
    }
    m_lastMwmPoints.clear();
  }

  vector<TMapRepr> m_usedMwms;
  size_t m_lastUsedMwm;
  vector<m2::PointD> m_lastMwmPoints;
};

template <class DataFacadeT> class MapsMePlugin final : public BasePlugin
{
    class GetByPoint
    {
        m2::PointD const &m_pt;
        std::vector<std::vector<m2::RegionD>> const &m_regions;

      public:
        size_t m_res;

        GetByPoint(std::vector<std::vector<m2::RegionD>> const &regions, m2::PointD const &pt)
            : m_pt(pt), m_regions(regions), m_res(-1)
        {
        }

        /// @param[in] id Index in m_countries.
        /// @return false If point is in country.
        bool operator()(size_t id)
        {
            auto it =
                find_if(m_regions[id].begin(), m_regions[id].end(), [&](m2::RegionD const &region)
                        { return region.Contains(m_pt);});
            if (it == m_regions[id].end())
                    return true;
            m_res = id;
            return false;
        }
    };

public:
    explicit MapsMePlugin(DataFacadeT *facade, std::string const &baseDir, osrm::NodeDataVectorT const & nodeData)
        : m_descriptorString("mapsme"), m_facade(facade),
          m_reader(baseDir + '/' + PACKED_POLYGONS_FILE),
          m_nodeData(nodeData)
    {
#ifndef MT_STRUCTURES
        SimpleLogger().Write(logWARNING) << "Multitreaded storage was not set on compile time!!! Do not use osrm-routed in several threads."
#endif
        ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
        rw::Read(src, m_countries);
        m_regions.resize(m_countries.size());
        for (size_t i = 0; i < m_countries.size(); ++i)
        {
            // load regions from file
            ReaderSource<ModelReaderPtr> src(m_reader.GetReader(strings::to_string(i)));

            uint32_t const count = ReadVarUint<uint32_t>(src);
            for (size_t j = 0; j < count; ++j)
            {
                vector<m2::PointD> points;
                serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);

                m_regions[i].emplace_back(move(m2::RegionD(points.begin(), points.end())));
            }
        }
        m_searchEngine = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
    }

    template <class ToDo> void ForEachCountry(m2::PointD const &pt, ToDo &toDo) const
    {
        for (size_t i = 0; i < m_countries.size(); ++i)
            if (m_countries[i].m_rect.IsPointInside(pt))
                if (!toDo(i))
                    return;
    }

    virtual ~MapsMePlugin() {}

    const std::string GetDescriptor() const override final { return m_descriptorString; }

    int HandleRequest(const RouteParameters &route_parameters, osrm::json::Object &reply) override final
    {
        double constexpr kMaxDistanceToFindMeters = 1000.0;

        //We process only two points case
        if (route_parameters.coordinates.size() != 2)
            return 400;

        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        std::vector<phantom_node_pair> phantom_node_pair_list(route_parameters.coordinates.size());

        for (const auto i : osrm::irange<std::size_t>(0, route_parameters.coordinates.size()))
        {
            std::vector<std::pair<PhantomNode, double>> phantom_node_vector;
            //FixedPointCoordinate &coordinate = route_parameters.coordinates[i];
            if (m_facade->IncrementalFindPhantomNodeForCoordinateWithMaxDistance(route_parameters.coordinates[i],
                                                                                 phantom_node_vector, kMaxDistanceToFindMeters,
                                                                                 0 /*min_number_of_phantom_nodes*/, 2 /*max_number_of_phantom_nodes*/))
            {
                BOOST_ASSERT(!phantom_node_vector.empty());
                // Don't know why, but distance may be higher that maxDistance.
                if (phantom_node_vector.front().second > kMaxDistanceToFindMeters)
                  continue;
                phantom_node_pair_list[i].first = phantom_node_vector.front().first;
                if (phantom_node_vector.size() > 1)
                {
                    if (phantom_node_vector.back().second > kMaxDistanceToFindMeters)
                      continue;
                    phantom_node_pair_list[i].second = phantom_node_vector.back().first;
                }
            }
        }

        auto check_component_id_is_tiny = [](const phantom_node_pair &phantom_pair)
        {
            return phantom_pair.first.component_id != 0;
        };

        const bool every_phantom_is_in_tiny_cc =
                std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                            check_component_id_is_tiny);

        // are all phantoms from a tiny cc?
        const auto component_id = phantom_node_pair_list.front().first.component_id;

        auto check_component_id_is_equal = [component_id](const phantom_node_pair &phantom_pair)
        {
            return component_id == phantom_pair.first.component_id;
        };

        const bool every_phantom_has_equal_id =
                std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                            check_component_id_is_equal);

        auto swap_phantom_from_big_cc_into_front = [](phantom_node_pair &phantom_pair)
        {
            if (0 != phantom_pair.first.component_id)
            {
                using namespace std;
                swap(phantom_pair.first, phantom_pair.second);
            }
        };

        // this case is true if we take phantoms from the big CC
        if (!every_phantom_is_in_tiny_cc || !every_phantom_has_equal_id)
        {
            std::for_each(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                          swap_phantom_from_big_cc_into_front);
        }

        InternalRouteResult raw_route;
        auto build_phantom_pairs =
           [&raw_route](const phantom_node_pair &first_pair, const phantom_node_pair &second_pair)
        {
            raw_route.segment_end_coordinates.emplace_back(
                PhantomNodes{first_pair.first, second_pair.first});
        };

        osrm::for_each_pair(phantom_node_pair_list, build_phantom_pairs);

        vector<bool> uturns;
        m_searchEngine->shortest_path(raw_route.segment_end_coordinates, uturns, raw_route);
        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
            return 400;
        }
        // Get mwm names
        UsedMwmChecker usedChecker;

        for (auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
            size_t const n = raw_route.unpacked_path_segments[i].size();
            for (size_t j = 0; j < n; ++j)
            {
                PathData const &path_data = raw_route.unpacked_path_segments[i][j];
                auto const & data = m_nodeData[path_data.node];
                if (data.m_segments.empty())
                    continue;
                auto const & seg = data.m_segments.front();
                m2::PointD pt = MercatorBounds::FromLatLon(seg.lat1, seg.lon1);
                GetByPoint doGet(m_regions, pt);
                ForEachCountry(pt, doGet);
                usedChecker.AddPoint(doGet.m_res, pt);
            }
        }

        osrm::json::Array json_array;
        for (auto & mwm : usedChecker.GetUsedMwms())
        {
            osrm::json::Array pointArray;
            pointArray.values.push_back(mwm.second.x);
            pointArray.values.push_back(mwm.second.y);
            pointArray.values.push_back(m_countries[mwm.first].m_countryId);
            json_array.values.push_back(pointArray);
        }
        reply.values["used_mwms"] = json_array;

        return 200;
    }

  private:
    std::unique_ptr<SearchEngine<DataFacadeT>> m_searchEngine;
    std::vector<storage::CountryDef> m_countries;
    std::vector<std::vector<m2::RegionD>> m_regions;
    std::string m_descriptorString;
    DataFacadeT * m_facade;
    FilesContainerR m_reader;
    osrm::NodeDataVectorT const & m_nodeData;
};
