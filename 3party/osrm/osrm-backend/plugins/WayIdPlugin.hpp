#pragma once

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
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

template <class DataFacadeT>
class WayIdPlugin final : public BasePlugin
{
public:
  WayIdPlugin(DataFacadeT * facade, osrm::NodeDataVectorT const & nodeData)
    : m_descriptorString("wayid"), m_facade(facade), m_nodeData(nodeData)
  {
#ifndef MT_STRUCTURES
    SimpleLogger().Write(logWARNING) << "Multitreaded storage was not set on compile time!!! Do "
                                        "not use osrm-routed in several threads."
#endif
    m_searchEngine = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);
  }

  const std::string GetDescriptor() const override final { return m_descriptorString; }
  int HandleRequest(const RouteParameters & route_parameters,
                    osrm::json::Object & reply) override final
  {
    double constexpr kMaxDistanceToFindMeters = 1000.0;

    // We process only two points case
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

    auto check_component_id_is_tiny = [](const phantom_node_pair & phantom_pair)
    {
      return phantom_pair.first.component_id != 0;
    };

    const bool every_phantom_is_in_tiny_cc =
        std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                    check_component_id_is_tiny);

    // are all phantoms from a tiny cc?
    const auto component_id = phantom_node_pair_list.front().first.component_id;

    auto check_component_id_is_equal = [component_id](const phantom_node_pair & phantom_pair)
    {
      return component_id == phantom_pair.first.component_id;
    };

    const bool every_phantom_has_equal_id =
        std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                    check_component_id_is_equal);

    auto swap_phantom_from_big_cc_into_front = [](phantom_node_pair & phantom_pair)
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
    auto build_phantom_pairs = [&raw_route](const phantom_node_pair & first_pair,
                                            const phantom_node_pair & second_pair)
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

    // Get ids of ways used in path.
    set<uint64_t> wayIds;

    for (auto i : osrm::irange<std::size_t>(0, raw_route.unpacked_path_segments.size()))
    {
      size_t const n = raw_route.unpacked_path_segments[i].size();
      for (size_t j = 0; j < n; ++j)
      {
        PathData const & path_data = raw_route.unpacked_path_segments[i][j];
        auto const & data = m_nodeData[path_data.node];
        for (auto const & seg : data.m_segments)
          wayIds.insert(seg.wayId);
      }
    }

    // Format answer.
    osrm::json::Array json_array;
    json_array.values.assign(wayIds.begin(), wayIds.end());
    reply.values["way_ids"] = move(json_array);

    return 200;
  }

private:
  std::unique_ptr<SearchEngine<DataFacadeT>> m_searchEngine;
  std::string m_descriptorString;
  DataFacadeT * m_facade;
  osrm::NodeDataVectorT const & m_nodeData;
};
