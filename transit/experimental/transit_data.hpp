#pragma once

#include "transit/experimental/transit_types_experimental.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/geo_object_id.hpp"
#include "base/visitor.hpp"

#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "cppjansson/cppjansson.hpp"

namespace transit
{
namespace experimental
{
using OsmIdToFeatureIdsMap = std::map<base::GeoObjectId, std::vector<FeatureId>>;
using EdgeIdToFeatureId = std::unordered_map<EdgeId, uint32_t, EdgeIdHasher>;
// Functions for parsing one line of line-by-line json and creating corresponding item in container.
void Read(base::Json const & obj, std::vector<Network> & networks);
void Read(base::Json const & obj, std::vector<Route> & routes);
void Read(base::Json const & obj, std::vector<Line> & lines);
void Read(base::Json const & obj, std::vector<LineMetadata> & linesMetadata);
void Read(base::Json const & obj, std::vector<Stop> & stops, OsmIdToFeatureIdsMap const & mapping);
void Read(base::Json const & obj, std::vector<Shape> & shapes);
void Read(base::Json const & obj, std::vector<Edge> & edges, EdgeIdToFeatureId & edgeFeatureIds);
void Read(base::Json const & obj, std::vector<Transfer> & transfers);
void Read(base::Json const & obj, std::vector<Gate> & gates, OsmIdToFeatureIdsMap const & mapping);

/// \brief The class contains all the information to make TRANSIT_FILE_TAG section.
class TransitData
{
public:
  void DeserializeFromJson(std::string const & dirWithJsons, OsmIdToFeatureIdsMap const & mapping);
  /// \note This method changes only |m_header| and fills it with correct offsets.
  void Serialize(Writer & writer);
  void Deserialize(Reader & reader);
  void DeserializeForRouting(Reader & reader);
  void DeserializeForRendering(Reader & reader);
  void DeserializeForCrossMwm(Reader & reader);
  void Clear();
  void CheckValid() const;
  void CheckSorted() const;
  void CheckUnique() const;
  bool IsEmpty() const;
  /// \brief Sorts all class fields by their ids.
  void Sort();
  void SetGatePedestrianSegments(size_t gateIdx, std::vector<SingleMwmSegment> const & seg);
  void SetStopPedestrianSegments(size_t stopIdx, std::vector<SingleMwmSegment> const & seg);

  std::vector<Stop> const & GetStops() const { return m_stops; }
  std::vector<Gate> const & GetGates() const { return m_gates; }
  std::vector<Edge> const & GetEdges() const { return m_edges; }
  std::vector<Transfer> const & GetTransfers() const { return m_transfers; }
  std::vector<Line> const & GetLines() const { return m_lines; }
  std::vector<LineMetadata> const & GetLinesMetadata() const { return m_linesMetadata; }
  std::vector<Shape> const & GetShapes() const { return m_shapes; }
  std::vector<Route> const & GetRoutes() const { return m_routes; }
  std::vector<Network> const & GetNetworks() const { return m_networks; }

  EdgeIdToFeatureId const & GetEdgeIdToFeatureId() const { return m_edgeFeatureIds; }

private:
  DECLARE_VISITOR_AND_DEBUG_PRINT(TransitData, visitor(m_stops, "stops"), visitor(m_gates, "gates"),
                                  visitor(m_edges, "edges"), visitor(m_transfers, "transfers"),
                                  visitor(m_lines, "lines"), visitor(m_shapes, "shapes"),
                                  visitor(m_networks, "networks"), visitor(m_routes, "routes"))
  friend TransitData FillTestTransitData();
  /// \brief Reads transit form |src|.
  /// \note Before calling any of the method except for ReadHeader() |m_header| has to be filled.
  void ReadHeader(NonOwningReaderSource & src);
  void ReadStops(NonOwningReaderSource & src);
  void ReadGates(NonOwningReaderSource & src);
  void ReadEdges(NonOwningReaderSource & src);
  void ReadTransfers(NonOwningReaderSource & src);
  void ReadLines(NonOwningReaderSource & src);
  void ReadLinesMetadata(NonOwningReaderSource & src);
  void ReadShapes(NonOwningReaderSource & src);
  void ReadRoutes(NonOwningReaderSource & src);
  void ReadNetworks(NonOwningReaderSource & src);

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
  std::vector<LineMetadata> m_linesMetadata;
  std::vector<Shape> m_shapes;

  EdgeIdToFeatureId m_edgeFeatureIds;
};
}  // namespace experimental
}  // namespace transit
