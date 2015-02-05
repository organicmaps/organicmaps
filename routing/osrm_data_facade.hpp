#pragma once

#include "../defines.hpp"

#include "../std/string.hpp"

#include "../base/bits.hpp"

#include "../coding/file_container.hpp"
#include "../coding/read_write_utils.hpp"

#include "../3party/succinct/elias_fano.hpp"
#include "../3party/succinct/elias_fano_compressed_list.hpp"
#include "../3party/succinct/gamma_vector.hpp"
#include "../3party/succinct/rs_bit_vector.hpp"
#include "../3party/succinct/mapper.hpp"

#include "../3party/osrm/osrm-backend/Server/DataStructures/BaseDataFacade.h"
#include "../3party/osrm/osrm-backend/DataStructures/TravelMode.h"

#include "../generator/routing_generator.hpp"
#include "cross_routing_context.hpp"


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

  uint32_t m_numberOfNodes;

public:

  void LoadRawData(char const * pRawEdgeData, char const * pRawEdgeIds, char const * pRawEdgeShortcuts, char const * pRawFanoMatrix)
  {
    ClearRawData();

    ASSERT(pRawEdgeData != nullptr, ());
    succinct::mapper::map(m_edgeData, pRawEdgeData);

    ASSERT(pRawEdgeIds != nullptr, ());
    succinct::mapper::map(m_edgeId, pRawEdgeIds);

    ASSERT(pRawEdgeShortcuts != nullptr, ());
    succinct::mapper::map(m_shortcuts, pRawEdgeShortcuts);

    ASSERT(pRawFanoMatrix != nullptr, ());
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

  virtual unsigned GetNumberOfNodes() const
  {
    return m_numberOfNodes;
  }

  virtual unsigned GetNumberOfEdges() const
  {
    return m_edgeData.size();
  }

  virtual unsigned GetOutDegree(const NodeID n) const
  {
    return EndEdges(n) - BeginEdges(n);
  }

  virtual NodeID GetTarget(const EdgeID e) const
  {
    return (m_matrix.select(e) / 2) % GetNumberOfNodes();
  }

  virtual EdgeDataT & GetEdgeData(const EdgeID e)
  {
    static EdgeDataT res;
    ASSERT(false, ("Maps me routing facade do not supports this edge unpacking method"));
    return res;
  }

  virtual EdgeDataT GetEdgeData(const EdgeID e, NodeID node)
  {
    EdgeDataT res;

    res.shortcut = m_shortcuts[e];
    res.id = res.shortcut ? (node - bits::ZigZagDecode(m_edgeId[m_shortcuts.rank(e)])) : 0;
    res.backward = (m_matrix.select(e) % 2 == 1);
    res.forward = !res.backward;
    res.distance = m_edgeData[e];

    return res;
  }

  virtual EdgeDataT & GetEdgeData(const EdgeID e) const
  {
    static EdgeDataT res;
    ASSERT(false, ("Maps me routing facade do not supports this  edge unpacking method"));
    return res;
  }

  //! TODO: Make proper travelmode getter when we add it to routing file
  virtual TravelMode GetTravelModeForEdgeID(const unsigned id) const
  {
      return TRAVEL_MODE_DEFAULT;
  }

  virtual EdgeID BeginEdges(const NodeID n) const
  {
    uint64_t idx = 2 * n * (uint64_t)GetNumberOfNodes();
    return n == 0 ? 0 : m_matrix.rank(min(idx, m_matrix.size()));
  }

  virtual EdgeID EndEdges(const NodeID n) const
  {
    uint64_t const idx = 2 * (n + 1) * (uint64_t)GetNumberOfNodes();
    return m_matrix.rank(min(idx, m_matrix.size()));
  }

  virtual EdgeRange GetAdjacentEdgeRange(const NodeID node) const
  {
    return osrm::irange(BeginEdges(node), EndEdges(node));
  }

  // searches for a specific edge
  virtual EdgeID FindEdge(const NodeID from, const NodeID to) const
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

  virtual EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const
  {
    return (EdgeID)0;
  }

  virtual EdgeID FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const
  {
    return (EdgeID)0;
  }

  // node and edge information access
  virtual FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const
  {
    return FixedPointCoordinate();
  }

  virtual bool EdgeIsCompressed(const unsigned id) const
  {
    return false;
  }

  virtual unsigned GetGeometryIndexForEdgeID(const unsigned id) const
  {
    return false;
  }

  virtual void GetUncompressedGeometry(const unsigned id, std::vector<unsigned> &result_nodes) const
  {
  }

  virtual TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const
  {
    return TurnInstruction::NoTurn;
  }

  virtual bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                          FixedPointCoordinate &result,
                                          const unsigned zoom_level = 18)
  {
    return false;
  }

  virtual bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                    PhantomNode &resulting_phantom_node,
                                    const unsigned zoom_level)
  {
    return false;
  }

  virtual bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                               std::vector<PhantomNode> &resulting_phantom_node_vector,
                                               const unsigned zoom_level,
                                               const unsigned number_of_results)
  {
    return false;
  }

  virtual unsigned GetCheckSum() const
  {
    return 0;
  }

  virtual unsigned GetNameIndexFromEdgeID(const unsigned id) const
  {
    return -1;
  }

  virtual void GetName(const unsigned name_id, std::string &result) const
  {
  }

  virtual std::string GetEscapedNameForNameID(const unsigned name_id) const
  {
    return std::string();
  }

  virtual std::string GetTimestamp() const
  {
    return "";
  }
};


template <class EdgeDataT> class OsrmDataFacade : public OsrmRawDataFacade<EdgeDataT>
{
  typedef OsrmRawDataFacade<EdgeDataT> super;
  CrossRoutingContext m_crossContext;

  FilesMappingContainer::Handle m_handleEdgeData;
  FilesMappingContainer::Handle m_handleEdgeId;
  FilesMappingContainer::Handle m_handleEdgeIdFano;
  FilesMappingContainer::Handle m_handleShortcuts;
  FilesMappingContainer::Handle m_handleFanoMatrix;

  using OsrmRawDataFacade<EdgeDataT>::LoadRawData;
  using OsrmRawDataFacade<EdgeDataT>::ClearRawData;
  uint32_t m_numberOfNodes;

public:
  OsrmDataFacade()
  {
  }

  void Load(FilesMappingContainer const & container)
  {
    Clear();

    m_handleEdgeData.Assign(container.Map(ROUTING_EDGEDATA_FILE_TAG));
    ASSERT(m_handleEdgeData.IsValid(), ());

    m_handleEdgeId.Assign(container.Map(ROUTING_EDGEID_FILE_TAG));
    ASSERT(m_handleEdgeId.IsValid(), ());

    m_handleShortcuts.Assign(container.Map(ROUTING_SHORTCUTS_FILE_TAG));
    ASSERT(m_handleShortcuts.IsValid(), ());

    m_handleFanoMatrix.Assign(container.Map(ROUTING_MATRIX_FILE_TAG));
    ASSERT(m_handleFanoMatrix.IsValid(), ());

    LoadRawData(m_handleEdgeData.GetData<char>(), m_handleEdgeId.GetData<char>(), m_handleShortcuts.GetData<char>(), m_handleFanoMatrix.GetData<char>());

    if (container.IsExist(ROUTING_CROSS_CONTEXT_TAG))
    {
      m_crossContext.Load(container.GetReader(ROUTING_CROSS_CONTEXT_TAG));
    }
    else
    {
      //LOG(LINFO, ("Old routing file version! Have no crossMwm information!"));
    }
  }

  void Clear()
  {
    ClearRawData();
    m_handleEdgeData.Unmap();
    m_handleEdgeId.Unmap();
    m_handleShortcuts.Unmap();
    m_handleFanoMatrix.Unmap();
  }

  CrossRoutingContext const & getRoutingContext()
  {
    return m_crossContext;
  }
};

}
