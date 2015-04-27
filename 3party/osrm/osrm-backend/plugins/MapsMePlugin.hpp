#pragma once

#include "plugin_base.hpp"
#include "../../../../base/string_utils.hpp"
#include "../../../../coding/file_container.hpp"
#include "../../../../coding/read_write_utils.hpp"
#include "../../../../defines.hpp"
#include "../../../../geometry/region2d.hpp"
#include "../../../../indexer/geometry_serialization.hpp"
#include "../../../../indexer/mercator.hpp"
#include "../../../../storage/country_decl.hpp"
#include "../../../../storage/country_polygon.hpp"
#include "../../../../base/logging.hpp"

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
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>


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
    explicit MapsMePlugin(DataFacadeT *facade, std::string const &baseDir, std::string const & nodeDataFile)
        : m_descriptorString("mapsme"), m_facade(facade),
          m_reader(baseDir + '/' + PACKED_POLYGONS_FILE)
    {
        if (!osrm::LoadNodeDataFromFile(nodeDataFile, m_nodeData))
        {
          SimpleLogger().Write(logDEBUG) << "Can't load node data";
          return;
        }
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
                serial::LoadOuterPath(src, serial::CodingParams(), points);

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
            std::vector<PhantomNode> phantom_node_vector;
            //FixedPointCoordinate &coordinate = route_parameters.coordinates[i];
            if (m_facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates[i],
                                                                phantom_node_vector, 1))
            {
        LOG(LINFO, ("FOUND", route_parameters.coordinates[i], phantom_node_vector));
                BOOST_ASSERT(!phantom_node_vector.empty());
                phantom_node_pair_list[i].first = phantom_node_vector.front();
                if (phantom_node_vector.size() > 1)
                {
                    phantom_node_pair_list[i].second = phantom_node_vector.back();
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

        LOG(LINFO, ("A"));

        InternalRouteResult raw_route;
        auto build_phantom_pairs =
           [&raw_route](const phantom_node_pair &first_pair, const phantom_node_pair &second_pair)
        {
            raw_route.segment_end_coordinates.emplace_back(
                PhantomNodes{first_pair.first, second_pair.first});
        };
       
        LOG(LINFO, ("B"));
        osrm::for_each_pair(phantom_node_pair_list, build_phantom_pairs);

        LOG(LINFO, ("B1", raw_route.segment_end_coordinates.front()));
        m_searchEngine->alternative_path(raw_route.segment_end_coordinates.front(), raw_route);

        LOG(LINFO, ("B2"));
        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }
        LOG(LINFO, ("C"));
        // Get mwm names
        vector<pair<string, m2::PointD>> usedMwms;

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

                if (doGet.m_res != -1)
                    usedMwms.emplace_back(make_pair(m_countries[doGet.m_res].m_name, pt));
            }
        }

        auto const it = std::unique(usedMwms.begin(), usedMwms.end(), [&](pair<string, m2::PointD> const & a, pair<string, m2::PointD> const & b) {return a.first == b.first;});
        usedMwms.erase(it, usedMwms.end());

        osrm::json::Array json_array;
        for (auto & mwm : usedMwms)
        {
            osrm::json::Array pointArray;
            pointArray.values.push_back(mwm.second.x);
            pointArray.values.push_back(mwm.second.y);
            pointArray.values.push_back(mwm.first);
            json_array.values.push_back(pointArray);
        }
        reply.values["used_mwms"] = json_array;

        //std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);
        //descriptor->SetConfig(route_parameters);
        //descriptor->Run(raw_route, rely);

        //JSON::render(reply.content, json_object);
        return 200;
    }

  private:
    std::unique_ptr<SearchEngine<DataFacadeT>> m_searchEngine;
    std::vector<storage::CountryDef> m_countries;
    std::vector<std::vector<m2::RegionD>> m_regions;
    std::string m_descriptorString;
    DataFacadeT * m_facade;
    FilesContainerR m_reader;
    osrm::NodeDataVectorT m_nodeData;
};

