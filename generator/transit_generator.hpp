#pragma once

#include "generator/osm_id.hpp"

#include "routing_common/transit_types.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "3party/jansson/myjansson.hpp"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace routing
{
namespace transit
{
using OsmIdToFeatureIdsMap = std::map<osm::Id, std::vector<FeatureId>>;

class DeserializerFromJson
{
public:
  DeserializerFromJson(json_struct_t* node, OsmIdToFeatureIdsMap const & osmIdToFeatureIds);

  template<typename T>
  typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value || std::is_same<T, double>::value>::type
      operator()(T & t, char const * name = nullptr)
  {
    GetField(t, name);
    return;
  }

  void operator()(std::string & s, char const * name = nullptr) { GetField(s, name); }
  void operator()(m2::PointD & p, char const * name = nullptr);
  void operator()(FeatureIdentifiers & id, char const * name = nullptr);
  void operator()(StopIdRanges & rs, char const * name = nullptr);

  template <typename T>
  void operator()(std::vector<T> & vs, char const * name = nullptr)
  {
    auto * arr = my::GetJSONOptionalField(m_node, name);
    if (arr == nullptr)
      return;

    if (!json_is_array(arr))
      MYTHROW(my::Json::Exception, ("The field", name, "must contain a json array."));
    size_t const sz = json_array_size(arr);
    vs.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
      DeserializerFromJson arrayItem(json_array_get(arr, i), m_osmIdToFeatureIds);
      arrayItem(vs[i]);
    }
  }

  template<typename T>
  typename std::enable_if<std::is_class<T>::value>::type operator()(T & t, char const * name = nullptr)
  {
    if (name != nullptr && json_is_object(m_node))
    {
      json_t * dictNode = my::GetJSONOptionalField(m_node, name);
      if (dictNode == nullptr)
        return; // No such field in json.

      DeserializerFromJson dict(dictNode, m_osmIdToFeatureIds);
      t.Visit(dict);
      return;
    }

    t.Visit(*this);
  }

private:
  template <typename T>
  void GetField(T & t, char const * name = nullptr)
  {
    if (name == nullptr)
    {
      // |name| is not set in case of array items
      FromJSON(m_node, t);
      return;
    }

    json_struct_t * field = my::GetJSONOptionalField(m_node, name);
    if (field == nullptr)
    {
      // No optional field |name| at |m_node|. In that case the default value should be set to |t|.
      // This default value is set at constructor of corresponding class which is filled with
      // |DeserializerFromJson|. And the value (|t|) is not changed at this method.
      return;
    }
    FromJSON(field, t);
  }

  json_struct_t * m_node;
  OsmIdToFeatureIdsMap const & m_osmIdToFeatureIds;
};

/// \brief the data contains all the information to make TRANSIT_FILE_TAG section.
class GraphData
{
public:
  // @todo(bykoianko) All the methods below should be rewrite with the help of visitors.
  void DeserializeFromJson(my::Json const & root, OsmIdToFeatureIdsMap const & mapping);
  void SerializeToMwm(std::string const & mwmPath) const;
  void Append(GraphData const & rhs);
  void Clear();
  bool IsValid() const;

  void SortStops();
  /// \brief Calculates and updates |m_edges| by adding valid value for Edge::m_weight
  /// if it's not valid.
  /// \note |m_stops|, Edge::m_stop1Id and Edge::m_stop2Id in |m_edges| should be valid before call.
  void CalculateEdgeWeight();
  /// \brief Calculates best pedestrian segment for every gate in |m_gates|.
  /// \note All gates in |m_gates| should have a valid |m_point| field before the call.
  void CalculateBestPedestrianSegment(string const & mwmPath, string const & countryId);

  std::vector<Stop> const & GetStops() const { return m_stops; }
  std::vector<Gate> const & GetGates() const { return m_gates; }
  std::vector<Edge> const & GetEdges() const { return m_edges; }
  std::vector<Transfer> const & GetTransfers() const { return m_transfers; }
  std::vector<Line> const & GetLines() const { return m_lines; }
  std::vector<Shape> const & GetShapes() const { return m_shapes; }
  std::vector<Network> const & GetNetworks() const { return m_networks; }

private:
  std::vector<Stop> m_stops;
  std::vector<Gate> m_gates;
  std::vector<Edge> m_edges;
  std::vector<Transfer> m_transfers;
  std::vector<Line> m_lines;
  std::vector<Shape> m_shapes;
  std::vector<Network> m_networks;
};

// @todo(bykoianko) Method below should be covered with tests.

/// \brief Fills |data| according to a transit graph (|transitJsonPath|).
/// \note Some of fields of |data| contains feature ids of a certain mwm. These fields are filled
/// iff the mapping (|osmIdToFeatureIdsPath|) contains them. If not the fields have default value.
void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, string const & transitJsonPath,
                         GraphData & data);

/// \brief Calculates and adds some information to transit graph (|data|) after deserializing
/// from json.
void ProcessGraph(string const & mwmPath, string const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data);

/// \brief Removes some items from |data| if they outside the mwm (|countryId|) border.
/// @todo(bykoinko) Certain rules which is used to clip the transit graph should be described here.
void ClipGraphByMwm(string const & mwmDir, string const & countryId, GraphData & data);

/// \brief Builds the transit section in the mwm.
/// \param mwmDir relative or full path to a directory where mwm is located.
/// \param countryId is an mwm name without extension of the processed mwm.
/// \param osmIdToFeatureIdsPath is a path to a file with osm id to feature ids mapping.
/// \param transitDir a path to directory with json files with transit graphs.
/// \note An mwm pointed by |mwmPath| should contain:
/// * feature geometry
/// * index graph (ROUTING_FILE_TAG)
void BuildTransit(string const & mwmDir, string const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir);
}  // namespace transit
}  // namespace routing
