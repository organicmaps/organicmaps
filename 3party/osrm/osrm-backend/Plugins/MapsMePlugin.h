#pragma once

#include "BasePlugin.h"

#include "../Algorithms/ObjectToBase64.h"
#include "../DataStructures/JSONContainer.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/SearchEngine.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Util/make_unique.hpp"
#include "../Util/StringUtil.h"
#include "../Util/TimingUtil.h"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "../../../../generator/country_loader.hpp"
#include "../../../../indexer/mercator.hpp"

template <class DataFacadeT> class MapsMePlugin final : public BasePlugin
{
  public:
    explicit MapsMePlugin(DataFacadeT *facade, std::string const & baseDir) : descriptor_string("mapsme"), facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
        CHECK(borders::LoadCountriesList(baseDir, m_countries),
            ("Error loading country polygons files"));
    }

    virtual ~MapsMePlugin() {}

    const std::string GetDescriptor() const final { return descriptor_string; }

    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply) final
    {
        // check number of parameters
        if (2 > route_parameters.coordinates.size())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData raw_route;
        raw_route.check_sum = facade->GetCheckSum();

        if (std::any_of(begin(route_parameters.coordinates),
                        end(route_parameters.coordinates),
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
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i], phantom_node_vector[i]);
                if (phantom_node_vector[i].isValid(facade->GetNumberOfNodes()))
                {
                    continue;
                }
            }
            facade->FindPhantomNodeForCoordinate(raw_route.raw_via_node_coordinates[i],
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

        search_engine_ptr->alternative_path(raw_route.segment_end_coordinates.front(), raw_route);

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }
        reply.status = http::Reply::ok;

        //Get mwm names
        set<string> usedMwms;

        for (auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
        {
          size_t const n = raw_route.unpacked_path_segments[i].size();
          for (size_t j = 0; j < n; ++j)
          {
            PathData const & path_data = raw_route.unpacked_path_segments[i][j];
            FixedPointCoordinate const coord = facade->GetCoordinateOfNode(path_data.node);
            string mwmName;
            m2::PointD mercatorPoint(MercatorBounds::LonToX(coord.lon/1000000.0), MercatorBounds::LatToY(coord.lat/1000000.0));
            m_countries.ForEachInRect(m2::RectD(mercatorPoint, mercatorPoint), [&](borders::CountryPolygons const & c)
            {
              bool inside = false;
              c.m_regions.ForEachInRect(m2::RectD(mercatorPoint, mercatorPoint), [&](m2::RegionD const & region)
              {
                if (region.Contains(mercatorPoint))
                  inside = true;
              });
              if (inside)
                mwmName = c.m_name;
            });
            usedMwms.insert(mwmName);
          }
        }

        JSON::Object json_object;
        JSON::Array json_array;
        json_array.values.insert(json_array.values.begin(), usedMwms.begin(), usedMwms.end());
        json_object.values["used_mwms"] = json_array;
        JSON::render(reply.content, json_object);
    }

  private:
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    borders::CountriesContainerT m_countries;
    std::string descriptor_string;
    DataFacadeT *facade;
};
