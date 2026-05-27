#pragma once

#include "transit/transit_types.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "base/visitor.hpp"

#include <glaze/json.hpp>

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace routing
{
namespace transit
{
using OsmIdToFeatureIdsMap = std::map<base::GeoObjectId, std::vector<FeatureId>>;
using JsonValue = glz::generic_u64;

inline JsonValue * GetJsonOptionalField(JsonValue * root, char const * field)
{
  CHECK(root != nullptr, ());
  auto * object = root->get_if<JsonValue::object_t>();
  if (object == nullptr)
    return nullptr;

  auto const it = object->find(field);
  if (it == object->end())
    return nullptr;
  return &it->second;
}

inline JsonValue * GetJsonObligatoryField(JsonValue * root, char const * field)
{
  auto * value = GetJsonOptionalField(root, field);
  CHECK(value != nullptr, ("Obligatory field", field, "is absent."));
  return value;
}

template <typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
void FromJsonValue(JsonValue const & root, T & result)
{
  CHECK(root.is_number(), ("Object must contain a json number."));
  result = root.as<T>();
}

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
void FromJsonValue(JsonValue const & root, T & result)
{
  std::underlying_type_t<T> value;
  FromJsonValue(root, value);
  result = static_cast<T>(value);
}

inline void FromJsonValue(JsonValue const & root, double & result)
{
  CHECK(root.is_number(), ("Object must contain a json number."));
  result = root.as<double>();
}

inline void FromJsonValue(JsonValue const & root, bool & result)
{
  CHECK(root.is_boolean(), ("Object must contain a boolean value."));
  result = root.get_boolean();
}

inline void FromJsonValue(JsonValue const & root, std::string & result)
{
  CHECK(root.is_string(), ("The field must contain a json string."));
  result = root.get_string();
}

class DeserializerFromJson
{
public:
  DeserializerFromJson(JsonValue * node, OsmIdToFeatureIdsMap const & osmIdToFeatureIds);

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value || std::is_same<T, double>::value>::type
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
    auto * arr = GetJsonOptionalField(m_node, name);
    if (arr == nullptr)
      return;

    auto * array = arr->get_if<JsonValue::array_t>();
    CHECK(array != nullptr, ("The field", name, "must contain a json array."));
    size_t const sz = array->size();
    vs.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
      DeserializerFromJson arrayItem(&(*array)[i], m_osmIdToFeatureIds);
      arrayItem(vs[i]);
    }
  }

  template <typename T>
  typename std::enable_if<std::is_class<T>::value && !std::is_same<T, Edge::WrappedEdgeId>::value &&
                          !std::is_same<T, Stop::WrappedStopId>::value>::type
  operator()(T & t, char const * name = nullptr)
  {
    if (name != nullptr && m_node->is_object())
    {
      JsonValue * dictNode = GetJsonOptionalField(m_node, name);
      if (dictNode == nullptr)
        return;  // No such field in json.

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
      FromJsonValue(*m_node, t);
      return;
    }

    JsonValue * field = GetJsonOptionalField(m_node, name);
    if (field == nullptr)
    {
      // No optional field |name| at |m_node|. In that case the default value should be set to |t|.
      // This default value is set at constructor of corresponding class which is filled with
      // |DeserializerFromJson|. And the value (|t|) is not changed at this method.
      return;
    }
    FromJsonValue(*field, t);
  }

  JsonValue * m_node;
  OsmIdToFeatureIdsMap const & m_osmIdToFeatureIds;
};

/// \brief The class contains all the information to make TRANSIT_FILE_TAG section.
class GraphData
{
public:
  void DeserializeFromJson(std::string_view json, OsmIdToFeatureIdsMap const & mapping);
  /// \note This method changes only |m_header| and fills it with correct offsets.
  void Serialize(Writer & writer);
  void DeserializeAll(Reader & reader);
  void DeserializeForRouting(Reader & reader);
  void DeserializeForRendering(Reader & reader);
  void DeserializeForCrossMwm(Reader & reader);
  void AppendTo(GraphData const & rhs);
  void Clear();
  void CheckValidSortedUnique() const;
  bool IsEmpty() const;

  /// \brief Sorts all class fields by their ids.
  void Sort();
  /// \brief Removes some items from all the class fields if they are outside |borders|.
  /// Please see description for the other Clip*() method for excact rules of clipping.
  /// \note Before call of the method every line in |m_stopIds| should contain |m_stopIds|
  /// with only one stop range.
  void ClipGraph(std::vector<m2::RegionD> const & borders);
  void SetGateBestPedestrianSegment(size_t gateIdx, SingleMwmSegment const & s);

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

  /// \brief Read a transit table form |srs|.
  /// \note Before calling any of the method except for ReadHeader() |m_header| has to be filled.
  void ReadHeader(NonOwningReaderSource & src);
  void ReadStops(NonOwningReaderSource & src);
  void ReadGates(NonOwningReaderSource & src);
  void ReadEdges(NonOwningReaderSource & src);
  void ReadTransfers(NonOwningReaderSource & src);
  void ReadLines(NonOwningReaderSource & src);
  void ReadShapes(NonOwningReaderSource & src);
  void ReadNetworks(NonOwningReaderSource & src);

  template <typename Fn>
  void DeserializeWith(Reader & reader, Fn && fn)
  {
    NonOwningReaderSource src(reader);
    ReadHeader(src);
    fn(src);
  }

  TransitHeader m_header;
  std::vector<Stop> m_stops;
  std::vector<Gate> m_gates;
  std::vector<Edge> m_edges;
  std::vector<Transfer> m_transfers;
  std::vector<Line> m_lines;
  std::vector<Shape> m_shapes;
  std::vector<Network> m_networks;
};
}  // namespace transit
}  // namespace routing
