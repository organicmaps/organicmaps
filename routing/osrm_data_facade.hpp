#pragma once

#include "routing/cross_routing_context.hpp"

#include "coding/file_container.hpp"
#include "coding/read_write_utils.hpp"

#include "base/bits.hpp"

#include "std/string.hpp"

#include "defines.hpp"

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/elias_fano_compressed_list.hpp"
#include "3party/succinct/gamma_vector.hpp"
#include "3party/succinct/rs_bit_vector.hpp"
#include "3party/succinct/mapper.hpp"

// TODO (ldragunov) exclude osrm specific headers from here! They causes "coordinate" problem
#include "3party/osrm/osrm-backend/server/data_structures/datafacade_base.hpp"
#include "3party/osrm/osrm-backend/data_structures/travel_mode.hpp"

namespace routing
{

template <class EdgeDataT> class OsrmRawDataFacade : public BaseDataFacade<EdgeDataT>
{
  template <class T> void ClearContainer(T & t)
  {
    T().swap(t);
  }

protected:
  succinct::elias_fano_compressed_list m_edgeData;
  succinct::rs_bit_vector m_shortcuts;
  succinct::elias_fano m_matrix;
  succinct::elias_fano_compressed_list m_edgeId;

  uint32_t m_numberOfNodes = 0;

public:
  //OsrmRawDataFacade(): m_numberOfNodes(0) {}

  void LoadRawData(char const * pRawEdgeData, char const * pRawEdgeIds, char const * pRawEdgeShortcuts, char const * pRawFanoMatrix)
  {
    ClearRawData();

    ASSERT(pRawEdgeData, ());
    succinct::mapper::map(m_edgeData, pRawEdgeData);

    ASSERT(pRawEdgeIds, ());
    succinct::mapper::map(m_edgeId, pRawEdgeIds);

    ASSERT(pRawEdgeShortcuts, ());
    succinct::mapper::map(m_shortcuts, pRawEdgeShortcuts);

    ASSERT(pRawFanoMatrix, ());
    m_numberOfNodes = *reinterpret_cast<uint32_t const *>(pRawFanoMatrix);
    succinct::mapper::map(m_matrix, pRawFanoMatrix + sizeof(m_numberOfNodes));
  }

  void ClearRawData()
  {
    ClearContainer(m_edgeData);
    ClearContainer(m_edgeId);
    ClearContainer(m_shortcuts);
    ClearContainer(m_matrix);
  }

  unsigned GetNumberOfNodes() const override
  {
    return m_numberOfNodes;
  }

  unsigned GetNumberOfEdges() const override
  {
    return static_cast<unsigned>(m_edgeData.size());
  }

  unsigned GetOutDegree(const NodeID n) const override
  {
    return EndEdges(n) - BeginEdges(n);
  }

  NodeID GetTarget(const EdgeID e) const override
  {
    return (m_matrix.select(e) / 2) % GetNumberOfNodes();
  }

  EdgeDataT GetEdgeData(const EdgeID e, NodeID node) const override
  {
    EdgeDataT res;

    res.shortcut = m_shortcuts[e];
    res.id = res.shortcut ? (node - static_cast<NodeID>(bits::ZigZagDecode(m_edgeId[static_cast<size_t>(m_shortcuts.rank(e))]))) : 0;
    res.backward = (m_matrix.select(e) % 2 == 1);
    res.forward = !res.backward;
    res.distance = static_cast<int>(m_edgeData[e]);

    return res;
  }

  EdgeDataT & GetEdgeData(const EdgeID) const override
  {
    static EdgeDataT res;
    ASSERT(false, ("Maps me routing facade does not supports this edge unpacking method"));
    return res;
  }

  //! TODO: Make proper travelmode getter when we add it to routing file
  TravelMode GetTravelModeForEdgeID(const unsigned) const override
  {
      return TRAVEL_MODE_DEFAULT;
  }

  EdgeID BeginEdges(const NodeID n) const override
  {
    uint64_t idx = 2 * n * (uint64_t)GetNumberOfNodes();
    return n == 0 ? 0 : static_cast<EdgeID>(m_matrix.rank(min(idx, m_matrix.size())));
  }

  EdgeID EndEdges(const NodeID n) const override
  {
    uint64_t const idx = 2 * (n + 1) * (uint64_t)GetNumberOfNodes();
    return static_cast<EdgeID>(m_matrix.rank(min(idx, m_matrix.size())));
  }

  EdgeRange GetAdjacentEdgeRange(const NodeID node) const override
  {
    return osrm::irange(BeginEdges(node), EndEdges(node));
  }

  // searches for a specific edge
  EdgeID FindEdge(const NodeID from, const NodeID to) const override
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

  EdgeID FindEdgeInEitherDirection(const NodeID, const NodeID) const override
  {
    return (EdgeID)0;
  }

  EdgeID FindEdgeIndicateIfReverse(const NodeID, const NodeID, bool &) const override
  {
    return (EdgeID)0;
  }

  // node and edge information access
  FixedPointCoordinate GetCoordinateOfNode(const unsigned) const override
  {
    return FixedPointCoordinate();
  }

  bool EdgeIsCompressed(const unsigned) const override
  {
    return false;
  }

  unsigned GetGeometryIndexForEdgeID(const unsigned) const override
  {
    return false;
  }

  void GetUncompressedGeometry(const unsigned, std::vector<unsigned> &) const override
  {
  }

  TurnInstruction GetTurnInstructionForEdgeID(const unsigned) const override
  {
    return TurnInstruction::NoTurn;
  }

  bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &,
                                          FixedPointCoordinate &,
                                          const unsigned = 18) override
  {
    return false;
  }

  /*bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                    PhantomNode &resulting_phantom_node,
                                    const unsigned zoom_level) override
  {
    return false;
  }*/

  /*bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                               std::vector<PhantomNode> &resulting_phantom_node_vector,
                                               const unsigned zoom_level,
                                               const unsigned number_of_results) override
  {
    return false;
  }*/

  bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &,
                                               std::vector<PhantomNode> &,
                                               const unsigned) override
  {
    return false;
  }

  bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &,
                                               PhantomNode &) override
  {
    return false;
  }

  bool IncrementalFindPhantomNodeForCoordinateWithMaxDistance(
           const FixedPointCoordinate &,
           std::vector<std::pair<PhantomNode, double>> &,
           const double,
           const unsigned,
           const unsigned) override
  {
    return false;
  }

  unsigned GetCheckSum() const override
  {
    return 0;
  }

  unsigned GetNameIndexFromEdgeID(const unsigned) const override
  {
    return -1;
  }

  //void GetName(const unsigned name_id, std::string &result) const override
  //{
  //}

  std::string get_name_for_id(const unsigned) const override
  {
    return std::string();
  }

  std::string GetTimestamp() const override
  {
    return std::string();
  }
};


template <class EdgeDataT> class OsrmDataFacade : public OsrmRawDataFacade<EdgeDataT>
{
  typedef OsrmRawDataFacade<EdgeDataT> super;

  FilesMappingContainer::Handle m_handleEdgeData;
  FilesMappingContainer::Handle m_handleEdgeId;
  FilesMappingContainer::Handle m_handleEdgeIdFano;
  FilesMappingContainer::Handle m_handleShortcuts;
  FilesMappingContainer::Handle m_handleFanoMatrix;

  using OsrmRawDataFacade<EdgeDataT>::LoadRawData;
  using OsrmRawDataFacade<EdgeDataT>::ClearRawData;

public:

  void Load(FilesMappingContainer const & container)
  {
    Clear();

    // Map huge data first, as we hope it will reduce fragmentation of the program address space.
    m_handleFanoMatrix.Assign(container.Map(ROUTING_MATRIX_FILE_TAG));
    ASSERT(m_handleFanoMatrix.IsValid(), ());

    m_handleEdgeData.Assign(container.Map(ROUTING_EDGEDATA_FILE_TAG));
    ASSERT(m_handleEdgeData.IsValid(), ());

    m_handleEdgeId.Assign(container.Map(ROUTING_EDGEID_FILE_TAG));
    ASSERT(m_handleEdgeId.IsValid(), ());

    m_handleShortcuts.Assign(container.Map(ROUTING_SHORTCUTS_FILE_TAG));
    ASSERT(m_handleShortcuts.IsValid(), ());

    LoadRawData(m_handleEdgeData.GetData<char>(), m_handleEdgeId.GetData<char>(), m_handleShortcuts.GetData<char>(), m_handleFanoMatrix.GetData<char>());
  }

  void Clear()
  {
    ClearRawData();
    m_handleEdgeData.Unmap();
    m_handleEdgeId.Unmap();
    m_handleShortcuts.Unmap();
    m_handleFanoMatrix.Unmap();
  }
};

}
