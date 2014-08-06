#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/iostream.hpp"

#include "../../../succinct/elias_fano.hpp"
#include "../../../succinct/gamma_vector.hpp"
#include "../../../succinct/bit_vector.hpp"
#include "../../../succinct/mapper.hpp"

//#include "../3party/osrm/osrm-backend/DataStructures/RangeTable.h"
#include "../3party/osrm/osrm-backend/DataStructures/StaticRTree.h"
#include "../3party/osrm/osrm-backend/DataStructures/EdgeBasedNode.h"
#include "../3party/osrm/osrm-backend/Server/DataStructures/BaseDataFacade.h"
#include "../3party/osrm/osrm-backend/DataStructures/OriginalEdgeData.h"

namespace routing
{

#define NOT_IMPLEMENTED { assert(false); }

template <class EdgeDataT> class OsrmDataFacade : public BaseDataFacade<EdgeDataT>
{
  typedef BaseDataFacade<EdgeDataT> super;
  typedef typename super::RTreeLeaf RTreeLeaf;

  boost::iostreams::mapped_file_source m_edgeDataSource;
  succinct::gamma_vector m_edgeData;
  boost::iostreams::mapped_file_source m_edgeIdSource;
  succinct::gamma_vector m_edgeId;
  boost::iostreams::mapped_file_source m_shortcutsSource;
  succinct::bit_vector m_shortcuts;

  succinct::elias_fano m_fanoMatrix;
  boost::iostreams::mapped_file_source m_fanoSource;

  unsigned m_numberOfNodes;

  // --- OSRM ---
  std::shared_ptr< vector<FixedPointCoordinate> > m_coordinateList;
  vector<NodeID> m_viaNodeList;
  vector<TurnInstruction> m_turnInstructionList;
  vector<unsigned> m_nameIDList;
  vector<bool> m_egdeIsCompressed;
  vector<unsigned> m_geometryIndices;
  vector<unsigned> m_geometryList;
  //RangeTable<16, false> m_name_table;
  //vector<char> m_names_char_list;
  StaticRTree<RTreeLeaf, vector<FixedPointCoordinate> > *m_staticRTree;

public:
  template <typename T>
  void loadFromFile(T & v, string const & fileName)
  {
    std::ifstream stream;
    stream.open(fileName);
    v.load(stream);
    stream.close();
  }

  OsrmDataFacade(string const & fileName)
    : m_staticRTree(0)
  {
    m_edgeDataSource.open(fileName + ".edgedata");
    succinct::mapper::map(m_edgeData, m_edgeDataSource);

    m_edgeIdSource.open(fileName + ".edgeid");
    succinct::mapper::map(m_edgeId, m_edgeIdSource);

    m_shortcutsSource.open(fileName + ".shortcuts");
    succinct::mapper::map(m_shortcuts, m_shortcutsSource);

    m_fanoSource.open(fileName + ".matrix");
    succinct::mapper::map(m_fanoMatrix, m_fanoSource);


    //std::cout << m_fanoMatrix.size() << std::endl;
    m_numberOfNodes = (unsigned)sqrt(m_fanoMatrix.size() / 2) + 1;

    // --- OSRM ---
    // load data
    LoadNodeAndEdgeInformation(fileName + ".nodes", fileName + ".edges");
    LoadGeometries(fileName + ".geometry");
    //LoadStreetNames(fileName + ".names");

    boost::filesystem::path ram_index_path(fileName + ".ramIndex");
    boost::filesystem::path file_index_path(fileName + ".fileIndex");
    m_staticRTree = new StaticRTree<RTreeLeaf, vector<FixedPointCoordinate> >(ram_index_path, file_index_path, m_coordinateList);
  }

  ~OsrmDataFacade()
  {
    delete m_staticRTree;

    m_edgeDataSource.close();
    m_edgeIdSource.close();
    m_shortcutsSource.close();
    m_fanoSource.close();
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
    NOT_IMPLEMENTED;
    return (EdgeID)0;
  }

  EdgeID FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const
  {
    NOT_IMPLEMENTED;
    return (EdgeID)0;
  }

  // node and edge information access
  FixedPointCoordinate GetCoordinateOfNode(const unsigned id) const
  {
    return m_coordinateList->at(id);
  }

  bool EdgeIsCompressed(const unsigned id) const
  {
    assert(id < m_egdeIsCompressed.size());
    return m_egdeIsCompressed[id];
  }

  unsigned GetGeometryIndexForEdgeID(const unsigned id) const
  {
    return m_viaNodeList.at(id);
  }

  void GetUncompressedGeometry(const unsigned id, std::vector<unsigned> &result_nodes) const
  {
    const unsigned begin = m_geometryIndices.at(id);
    const unsigned end = m_geometryIndices.at(id + 1);

    result_nodes.clear();
    result_nodes.insert(result_nodes.begin(), m_geometryList.begin() + begin, m_geometryList.begin() + end);
  }

  TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const
  {
    return m_turnInstructionList[id];
  }

  bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                          FixedPointCoordinate &result,
                                          const unsigned zoom_level = 18)
  {
    return m_staticRTree->LocateClosestEndPointForCoordinate(input_coordinate, result, zoom_level);
  }

  bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                    PhantomNode &resulting_phantom_node,
                                    const unsigned zoom_level)
  {

    return m_staticRTree->FindPhantomNodeForCoordinate(input_coordinate, resulting_phantom_node, zoom_level);
  }

  bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                               std::vector<PhantomNode> &resulting_phantom_node_vector,
                                               const unsigned zoom_level,
                                               const unsigned number_of_results)
  {
    NOT_IMPLEMENTED;
    return false;
  }

  unsigned GetCheckSum() const
  {
    NOT_IMPLEMENTED;
    return 0;
  }

  unsigned GetNameIndexFromEdgeID(const unsigned id) const
  {
    return m_nameIDList.at(id);
  }

  void GetName(const unsigned name_id, std::string &result) const
  {
    NOT_IMPLEMENTED;
  }

  std::string GetEscapedNameForNameID(const unsigned name_id) const
  {
      std::string temporary_string;
      GetName(name_id, temporary_string);
      return EscapeJSONString(temporary_string);
  }

  std::string GetTimestamp() const
  {
    return "";
  }


  // ------------------- OSRM ------------------------
  void LoadNodeAndEdgeInformation(string const & nodesFile, string const & edgesFile)
  {
    std::ifstream nodes_input_stream(nodesFile, std::ios::binary);

    NodeInfo current_node;
    unsigned number_of_coordinates = 0;
    nodes_input_stream.read((char *)&number_of_coordinates, sizeof(unsigned));
    m_coordinateList = std::make_shared< std::vector<FixedPointCoordinate> >(number_of_coordinates);
    for (unsigned i = 0; i < number_of_coordinates; ++i)
    {
      nodes_input_stream.read((char *)&current_node, sizeof(NodeInfo));
      m_coordinateList->at(i) = FixedPointCoordinate(current_node.lat, current_node.lon);
    }
    nodes_input_stream.close();

    std::ifstream edges_input_stream(edgesFile, std::ios::binary);
    unsigned number_of_edges = 0;
    edges_input_stream.read((char *)&number_of_edges, sizeof(unsigned));
    m_viaNodeList.resize(number_of_edges);
    m_nameIDList.resize(number_of_edges);
    m_turnInstructionList.resize(number_of_edges);
    m_egdeIsCompressed.resize(number_of_edges);

    unsigned compressed = 0;

    OriginalEdgeData current_edge_data;
    for (unsigned i = 0; i < number_of_edges; ++i)
    {
      edges_input_stream.read((char *)&(current_edge_data), sizeof(OriginalEdgeData));
      m_viaNodeList[i] = current_edge_data.via_node;
      m_nameIDList[i] = current_edge_data.name_id;
      m_turnInstructionList[i] = current_edge_data.turn_instruction;
      m_egdeIsCompressed[i] = current_edge_data.compressed_geometry;
      if (m_egdeIsCompressed[i])
      {
          ++compressed;
      }
    }

    edges_input_stream.close();
  }

  void LoadGeometries(string const & geometry_file)
  {
    std::ifstream geometry_stream(geometry_file, std::ios::binary);
    unsigned number_of_indices = 0;
    unsigned number_of_compressed_geometries = 0;

    geometry_stream.read((char *)&number_of_indices, sizeof(unsigned));

    m_geometryIndices.resize(number_of_indices);
    if (number_of_indices > 0)
      geometry_stream.read((char *)&(m_geometryIndices[0]), number_of_indices * sizeof(unsigned));

    geometry_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));

    BOOST_ASSERT(m_geometryIndices.back() == number_of_compressed_geometries);
    m_geometryList.resize(number_of_compressed_geometries);

    if (number_of_compressed_geometries > 0)
        geometry_stream.read((char *)&(m_geometryList[0]), number_of_compressed_geometries * sizeof(unsigned));

    geometry_stream.close();
  }

  /*void LoadStreetNames(string const & namesFile)
  {
    std::ifstream name_stream(namesFile, std::ios::binary);

    name_stream >> m_name_table;

    unsigned number_of_chars = 0;
    name_stream.read((char *)&number_of_chars, sizeof(unsigned));
    ASSERT(0 != number_of_chars, "name file broken");
    m_names_char_list.resize(number_of_chars + 1); //+1 gives sentinel element
    name_stream.read((char *)&m_names_char_list[0], number_of_chars * sizeof(char));
    name_stream.close();
  }*/


};

}
