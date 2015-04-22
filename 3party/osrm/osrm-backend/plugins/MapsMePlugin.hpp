#pragma once

#include "plugin_base.hpp"

/*
#include "../algorithms/object_encoder.h"
#include "../DataStructures/EdgeBasedNodeData.h"
#include "../DataStructures/JSONContainer.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/SearchEngine.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Util/make_unique.hpp"
#include "../Util/StringUtil.h"
#include "../Util/TimingUtil.h"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "../../../../base/string_utils.hpp"
#include "../../../../coding/file_container.hpp"
#include "../../../../coding/read_write_utils.hpp"
#include "../../../../defines.hpp"
#include "../../../../geometry/region2d.hpp"
#include "../../../../indexer/geometry_serialization.hpp"
#include "../../../../indexer/mercator.hpp"
#include "../../../../storage/country_decl.hpp"
#include "../../../../storage/country_polygon.hpp"

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

    const std::string GetDescriptor() const final { return m_descriptorString; }

    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply) final
    {
        // check number of parameters
        if (2 > route_parameters.coordinates.size())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData raw_route;
        raw_route.check_sum = m_facade->GetCheckSum();

        if (std::any_of(begin(route_parameters.coordinates), end(route_parameters.coordinates),
                        [&](FixedPointCoordinate coordinate)
                        {
                return !coordinate.isValid();
            }))
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        for (const FixedPointCoordinate &coordinate : route_parameters.coordinates)
        {
            raw_route.raw_via_node_coordinates.emplace_back(coordinate);
        }

        std::vector<PhantomNode> phantom_node_vector(raw_route.raw_via_node_coordinates.size());
        const bool checksum_OK = (route_parameters.check_sum == raw_route.check_sum);

        for (unsigned i = 0; i < raw_route.raw_via_node_coordinates.size(); ++i)
        {
            m_facade->FindPhantomNodeForCoordinate(raw_route.raw_via_node_coordinates[i],
                                                 phantom_node_vector[i],
                                                 route_parameters.zoom_level);
        }

        PhantomNodes current_phantom_node_pair;
        for (unsigned i = 0; i < phantom_node_vector.size() - 1; ++i)
        {
            current_phantom_node_pair.source_phantom = phantom_node_vector[i];
            current_phantom_node_pair.target_phantom = phantom_node_vector[i + 1];
            raw_route.segment_end_coordinates.emplace_back(current_phantom_node_pair);
        }

        m_searchEngine->alternative_path(raw_route.segment_end_coordinates.front(), raw_route);

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }
        reply.status = http::Reply::ok;

        // Get mwm names
        set<string> usedMwms;

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
                    usedMwms.insert(m_countries[doGet.m_res].m_name);
            }
        }

        JSON::Object json_object;
        JSON::Array json_array;
        json_array.values.insert(json_array.values.begin(), usedMwms.begin(), usedMwms.end());
        json_object.values["used_mwms"] = json_array;
        JSON::render(reply.content, json_object);
    }

  private:
    std::unique_ptr<SearchEngine<DataFacadeT>> m_searchEngine;
    std::vector<storage::CountryDef> m_countries;
    std::vector<std::vector<m2::RegionD>> m_regions;
    std::string m_descriptorString;
    DataFacadeT * m_facade;
    FilesContainerR m_reader;
    osrm::NodeDataVectorT m_nodeData;
};*/

