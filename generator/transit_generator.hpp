#pragma once

#include "routing_common/transit_types.hpp"

#include "storage/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include "coding/file_writer.hpp"

#include "base/macros.hpp"
#include "base/osm_id.hpp"

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

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value ||
                          std::is_same<T, double>::value>::type
  operator()(T & t, char const * name = nullptr)
  {
    GetField(t, name);
    return;
  }

  void operator()(std::string & s, char const * name = nullptr) { GetField(s, name); }
  void operator()(m2::PointD & p, char const * name = nullptr);
  void operator()(FeatureIdentifiers & id, char const * name = nullptr);
  void operator()(EdgeFlags & edgeFlags, char const * name = nullptr);
  void operator()(StopIdRanges & rs, char const * name = nullptr);

  template <typename T>
  typename std::enable_if<std::is_same<T, Edge::WrappedEdgeId>::value ||
      std::is_same<T, Stop::WrappedStopId>::value>::type
  operator()(T & t, char const * name = nullptr)
  {
    typename T::RepType id;
    operator()(id, name);
    t.Set(id);
  }

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

  template <typename T>
  typename std::enable_if<std::is_class<T>::value &&
                          !std::is_same<T, Edge::WrappedEdgeId>::value &&
                          !std::is_same<T, Stop::WrappedStopId>::value>::type
  operator()(T & t, char const * name = nullptr)
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

/// \brief The class contains all the information to make TRANSIT_FILE_TAG section.
class GraphData
{
public:
  void DeserializeFromJson(my::Json const & root, OsmIdToFeatureIdsMap const & mapping);
  void SerializeToMwm(FileWriter & writer) const;
  void AppendTo(GraphData const & rhs);
  void Clear();
  bool IsValid() const;
  bool IsEmpty() const;

  /// \brief Sorts all class fields by their ids.
  void Sort();
  /// \brief Removes some items from all the class fields if they are outside |borders|.
  /// Please see description for the other Clip*() method for excact rules of clipping.
  /// \note Before call of the method every line in |m_stopIds| should contain |m_stopIds|
  /// with only one stop range.
  void ClipGraph(std::vector<m2::RegionD> const & borders);
  /// \brief Calculates best pedestrian segment for every gate in |m_gates|.
  /// \note All gates in |m_gates| must have a valid |m_point| field before the call.
  void CalculateBestPedestrianSegments(std::string const & mwmPath, std::string const & countryId);

  std::vector<Stop> const & GetStops() const { return m_stops; }
  std::vector<Gate> const & GetGates() const { return m_gates; }
  std::vector<Edge> const & GetEdges() const { return m_edges; }
  std::vector<Transfer> const & GetTransfers() const { return m_transfers; }
  std::vector<Line> const & GetLines() const { return m_lines; }
  std::vector<Shape> const & GetShapes() const { return m_shapes; }
  std::vector<Network> const & GetNetworks() const { return m_networks; }

private:
  DECLARE_VISITOR_AND_DEBUG_PRINT(GraphData, visitor(m_stops, "stops"), visitor(m_gates, "gates"),
                                  visitor(m_edges, "edges"), visitor(m_transfers, "transfers"),
                                  visitor(m_lines, "lines"), visitor(m_shapes, "shapes"),
                                  visitor(m_networks, "networks"))

  bool IsUnique() const;
  bool IsSorted() const;

  /// \brief Clipping |m_lines| with |borders|.
  /// \details After a call of the method the following stop ids in |m_lines| are left:
  /// * stops inside |borders|
  /// * stops which are connected with an edge from |m_edges| with stops inside |borders|
  /// \note Lines without stops are removed from |m_lines|.
  /// \note Before call of the method every line in |m_lines| should contain |m_stopIds|
  /// with only one stop range.
  void ClipLines(std::vector<m2::RegionD> const & borders);
  /// \brief Removes all stops from |m_stops| which are not contained in |m_lines| at field |m_stopIds|.
  /// \note Only stops which stop ids contained in |m_lines| will left in |m_stops|
  /// after call of this method.
  void ClipStops();
  /// \brief Removes all networks from |m_networks| which are not contained in |m_lines|
  /// at field |m_networkId|.
  void ClipNetworks();
  /// \brief Removes gates from |m_gates| if there's no stop in |m_stops| with their stop ids.
  void ClipGates();
  /// \brief Removes transfers from |m_transfers| if there's no stop in |m_stops| with their stop ids.
  void ClipTransfer();
  /// \brief Removes edges from |m_edges| if their ends are not contained in |m_stops|.
  void ClipEdges();
  /// \brief Removes all shapes from |m_shapes| which are not reffered form |m_edges|.
  void ClipShapes();

  std::vector<Stop> m_stops;
  std::vector<Gate> m_gates;
  std::vector<Edge> m_edges;
  std::vector<Transfer> m_transfers;
  std::vector<Line> m_lines;
  std::vector<Shape> m_shapes;
  std::vector<Network> m_networks;
};

/// \brief Fills |data| according to a transit graph (|transitJsonPath|).
/// \note Some fields of |data| contain feature ids of a certain mwm. These fields are filled
/// iff the mapping (|osmIdToFeatureIdsPath|) contains them. Otherwise the fields have default value.
void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, std::string const & transitJsonPath,
                         GraphData & data);

/// \brief Calculates and adds some information to transit graph (|data|) after deserializing
/// from json.
void ProcessGraph(std::string const & mwmPath, storage::TCountryId const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data);

/// \brief Builds the transit section in the mwm based on transit graph in json which represents
/// trasit graph clipped by the mwm borders.
/// \param mwmDir relative or full path to a directory where mwm is located.
/// \param countryId is an mwm name without extension of the processed mwm.
/// \param osmIdToFeatureIdsPath is a path to a file with osm id to feature ids mapping.
/// \param transitDir relative or full path to a directory with json files of transit graphs.
/// It's assumed that the files have the same name with country ids and extension TRANSIT_FILE_EXTENSION.
/// \note An mwm pointed by |mwmPath| should contain:
/// * feature geometry
/// * index graph (ROUTING_FILE_TAG)
void BuildTransit(std::string const & mwmDir, storage::TCountryId const & countryId,
                  std::string const & osmIdToFeatureIdsPath, std::string const & transitDir);
}  // namespace transit
}  // namespace routing
