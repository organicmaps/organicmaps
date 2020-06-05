#pragma once

#include "transit/experimental/transit_types_experimental.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "base/visitor.hpp"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace transit
{
namespace experimental
{
using OsmIdToFeatureIdsMap = std::map<base::GeoObjectId, std::vector<FeatureId>>;

/// \brief The class contains all the information to make TRANSIT_FILE_TAG section.
class TransitData
{
public:
  void DeserializeFromJson(std::string const & dirWithJsons, OsmIdToFeatureIdsMap const & mapping);
  /// \note This method changes only |m_header| and fills it with correct offsets.
  // TODO(o.khlopkova) Add implementation for methods:
  void Serialize(Writer & writer) {}
  void DeserializeAll(Reader & reader) {}
  void DeserializeForRouting(Reader & reader) {}
  void DeserializeForRendering(Reader & reader) {}
  void DeserializeForCrossMwm(Reader & reader) {}
  void Clear() {}
  void CheckValidSortedUnique() const {}
  bool IsEmpty() const { return false; }
  /// \brief Sorts all class fields by their ids.
  void Sort() {}
  void SetGateBestPedestrianSegment(size_t gateIdx, SingleMwmSegment const & s) {}

  std::vector<Stop> const & GetStops() const { return m_stops; }
  std::vector<Gate> const & GetGates() const { return m_gates; }
  std::vector<Edge> const & GetEdges() const { return m_edges; }
  std::vector<Transfer> const & GetTransfers() const { return m_transfers; }
  std::vector<Line> const & GetLines() const { return m_lines; }
  std::vector<Shape> const & GetShapes() const { return m_shapes; }
  std::vector<Network> const & GetNetworks() const { return m_networks; }

private:
  DECLARE_VISITOR_AND_DEBUG_PRINT(TransitData, visitor(m_stops, "stops"), visitor(m_gates, "gates"),
                                  visitor(m_edges, "edges"), visitor(m_transfers, "transfers"),
                                  visitor(m_lines, "lines"), visitor(m_shapes, "shapes"),
                                  visitor(m_networks, "networks"), visitor(m_routes, "routes"))

  /// \brief Read a transit table form |src|.
  /// \note Before calling any of the method except for ReadHeader() |m_header| has to be filled.
  // TODO(o.khlopkova) Add implementation for methods:
  void ReadHeader(NonOwningReaderSource & src) {}
  void ReadStops(NonOwningReaderSource & src) {}
  void ReadGates(NonOwningReaderSource & src) {}
  void ReadEdges(NonOwningReaderSource & src) {}
  void ReadTransfers(NonOwningReaderSource & src) {}
  void ReadLines(NonOwningReaderSource & src) {}
  void ReadShapes(NonOwningReaderSource & src) {}
  void ReadNetworks(NonOwningReaderSource & src) {}
  void ReadRoutes(NonOwningReaderSource & src) {}

  template <typename Fn>
  void DeserializeWith(Reader & reader, Fn && fn)
  {
    NonOwningReaderSource src(reader);
    ReadHeader(src);
    fn(src);
  }

  TransitHeader m_header;
  std::vector<Network> m_networks;
  std::vector<Route> m_routes;
  std::vector<Stop> m_stops;
  std::vector<Gate> m_gates;
  std::vector<Edge> m_edges;
  std::vector<Transfer> m_transfers;
  std::vector<Line> m_lines;
  std::vector<Shape> m_shapes;
};
}  // namespace experimental
}  // namespace transit
