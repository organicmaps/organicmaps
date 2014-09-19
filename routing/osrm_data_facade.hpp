#pragma once

#include "../defines.hpp"

#include "../std/string.hpp"

#include "../coding/file_container.hpp"

#include "../../../succinct/elias_fano.hpp"
#include "../../../succinct/elias_fano_compressed_list.hpp"
#include "../../../succinct/gamma_vector.hpp"
#include "../../../succinct/bit_vector.hpp"
#include "../../../succinct/mapper.hpp"

#include "../3party/osrm/osrm-backend/Server/DataStructures/BaseDataFacade.h"


namespace routing
{

template <class EdgeDataT> class OsrmDataFacade : public BaseDataFacade<EdgeDataT>
{
  typedef BaseDataFacade<EdgeDataT> super;

  succinct::elias_fano_compressed_list m_edgeData;
  succinct::bit_vector m_shortcuts;
  succinct::elias_fano m_fanoMatrix;
  succinct::elias_fano_compressed_list m_edgeId;

  FilesMappingContainer const & m_container;

  FilesMappingContainer::Handle m_handleEdgeData;
  FilesMappingContainer::Handle m_handleEdgeId;
  FilesMappingContainer::Handle m_handleShortcuts;
  FilesMappingContainer::Handle m_handleFanoMatrix;

  unsigned m_numberOfNodes;

public:
  OsrmDataFacade(FilesMappingContainer const & container)
    : m_container(container)
  {
    m_handleEdgeData.Assign(m_container.Map(ROUTING_EDGEDATA_FILE_TAG));
    ASSERT(m_handleEdgeData.IsValid(), ());
    succinct::mapper::map(m_edgeData, m_handleEdgeData.GetData<char>());

    m_handleEdgeId.Assign(m_container.Map(ROUTING_EDGEID_FILE_TAG));
    ASSERT(m_handleEdgeId.IsValid(), ());
    succinct::mapper::map(m_edgeId, m_handleEdgeId.GetData<char>());

    m_handleShortcuts.Assign(m_container.Map(ROUTING_SHORTCUTS_FILE_TAG));
    ASSERT(m_handleShortcuts.IsValid(), ());
    succinct::mapper::map(m_shortcuts, m_handleShortcuts.GetData<char>());

    m_handleFanoMatrix.Assign(m_container.Map(ROUTING_MATRIX_FILE_TAG));
    ASSERT(m_handleFanoMatrix.IsValid(), ());
    succinct::mapper::map(m_fanoMatrix, m_handleFanoMatrix.GetData<char>());

    m_numberOfNodes = (unsigned)sqrt(m_fanoMatrix.size() / 2) + 1;
  }


  unsigned GetNumberOfNodes() const
  {
    return m_numberOfNodes;
  }

  unsigned GetNumberOfEdges() const
  {
    return m_edgeData.size();
  }

  unsigned GetOutDegree(const NodeID n) const
  {
    return EndEdges(n) - BeginEdges(n);
  }

  NodeID GetTarget(const EdgeID e) const
  {
    return (m_fanoMatrix.select(e) / 2) % GetNumberOfNodes();
  }

  //! TODO: Remove static variable
  EdgeDataT & GetEdgeData(const EdgeID e)
  {
    static EdgeDataT res;

    uint64_t data = m_edgeData[e];

    res.id = m_edgeId[e];
    res.backward = data & 0x1;
    data >>= 1;
    res.forward = data & 0x1;
    data >>= 1;
    res.distance = data;
    res.shortcut = m_shortcuts[e];

    return res;
  }

  //! TODO: Remove static variable
  EdgeDataT const & GetEdgeData(const EdgeID e) const
  {
    static EdgeDataT res;

    uint64_t data = m_edgeData[e];

    res.id = m_edgeId[e];
    res.backward = data & 0x1;
    data >>= 1;
    res.forward = data & 0x1;
    data >>= 1;
    res.distance = data;
    res.shortcut = m_shortcuts[e];

    return res;
  }

  EdgeID BeginEdges(const NodeID n) const
  {
    return n == 0 ? 0 : m_fanoMatrix.rank(2 * n * (uint64_t)GetNumberOfNodes());
  }

  EdgeID EndEdges(const NodeID n) const
  {
    uint64_t const idx = 2 * (n + 1) * (uint64_t)GetNumberOfNodes();
    return m_fanoMatrix.rank(std::min(idx, m_fanoMatrix.size()));
  }

  EdgeRange GetAdjacentEdgeRange(const NodeID node) const
  {
    return osrm::irange(BeginEdges(node), EndEdges(node));
  }

  // searches for a specific edge
  EdgeID FindEdge(const NodeID from, const NodeID to) const
  {
    EdgeID smallest_edge = SPECIAL_EDGEID;
    EdgeWeight smallest_weight = INVALID_EDGE_WEIGHT;
    for (auto edge : GetAdjacentEdgeRange(from))
    {
      const NodeID target = GetTarget(edge);
      const EdgeWeight weight = GetEdgeData(edge).distance;
      if (target == to && weight < smallest_weight)
      {
        smallest_edge = edge;
        smallest_weight = weight;
      }
    }
    return smallest_edge;
  }

  EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const
  {
    return (EdgeID)0;
  }

  EdgeID FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const
  {
    return (EdgeID)0;
  }

  // node and edge information access
  FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const
  {
    return FixedPointCoordinate();
  }

  bool EdgeIsCompressed(const unsigned id) const
  {
    return false;
  }

  unsigned GetGeometryIndexForEdgeID(const unsigned id) const
  {
    return false;
  }

  void GetUncompressedGeometry(const unsigned id, std::vector<unsigned> &result_nodes) const
  {
  }

  TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const
  {
    return TurnInstruction::NoTurn;
  }

  bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                          FixedPointCoordinate &result,
                                          const unsigned zoom_level = 18)
  {
    return false;
  }

  bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                    PhantomNode &resulting_phantom_node,
                                    const unsigned zoom_level)
  {
    return false;
  }

  bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                               std::vector<PhantomNode> &resulting_phantom_node_vector,
                                               const unsigned zoom_level,
                                               const unsigned number_of_results)
  {
    return false;
  }

  unsigned GetCheckSum() const
  {
    return 0;
  }

  unsigned GetNameIndexFromEdgeID(const unsigned id) const
  {
    return -1;
  }

  void GetName(const unsigned name_id, std::string &result) const
  {
  }

  std::string GetEscapedNameForNameID(const unsigned name_id) const
  {
    return std::string();
  }

  std::string GetTimestamp() const
  {
    return "";
  }
};

}
