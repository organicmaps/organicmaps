#include "converter.hpp"

#include <iostream>

#include "../Server/DataStructures/InternalDataFacade.h"
#include "../DataStructures/QueryEdge.h"

#include "../../../../coding/matrix_traversal.hpp"
//#include "../../../../routing/osrm_data_facade.hpp"

#include "../../../succinct/elias_fano.hpp"
#include "../../../succinct/gamma_vector.hpp"
#include "../../../succinct/mapper.hpp"


namespace  mapsme
{

void PrintStatus(bool b)
{
  std::cout << (b ? "[Ok]" : "[Fail]") << std::endl;
}

Converter::Converter()
{

}

void Converter::run(const std::string & name)
{
  ServerPaths server_paths;

  server_paths["hsgrdata"] = boost::filesystem::path(name + ".hsgr");
  server_paths["ramindex"] = boost::filesystem::path(name + ".ramIndex");
  server_paths["fileindex"] = boost::filesystem::path(name + ".fileIndex");
  server_paths["geometries"] = boost::filesystem::path(name + ".geometry");
  server_paths["nodesdata"] = boost::filesystem::path(name + ".nodes");
  server_paths["edgesdata"] = boost::filesystem::path(name + ".edges");
  server_paths["namesdata"] = boost::filesystem::path(name + ".names");
  server_paths["timestamp"] = boost::filesystem::path(name + ".timestamp");

  std::cout << "Create internal data facade for file: " << name << "...";
  InternalDataFacade<QueryEdge::EdgeData> facade(server_paths);
  PrintStatus(true);

  uint64_t nodeCount = facade.GetNumberOfNodes();

  std::vector<uint64_t> edges;
  std::vector<int32_t> edgesData;
  std::vector<bool> shortcuts;
  std::vector<int32_t> edgeId;

  std::cout << "Repack graph...";

  for (uint64_t node = 0; node < nodeCount; ++node)
  {
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
      uint64_t target = facade.GetTarget(edge);
      auto const & data = facade.GetEdgeData(edge);

      assert(data.forward || data.backward);

      uint32_t d = data.distance;
      d = d << 1;
      d += data.forward ? 1 : 0;
      d = d << 1;
      d += data.backward ? 1 : 0;

      edges.push_back(TraverseMatrixInRowOrder<uint64_t>(nodeCount, node, target, data.backward));
      edgesData.push_back(d);
      shortcuts.push_back(data.shortcut);
      edgeId.push_back(data.id);
    }
  }
  std::cout << "Edges count: " << edgeId.size() << std::endl;
  PrintStatus(true);

  std::cout << "Sort edges...";
  std::sort(edges.begin(), edges.end());
  PrintStatus(true);

  std::cout << "Edges count: " << edges.size() << std::endl;

  std::cout << "--- Save matrix" << std::endl;
  succinct::elias_fano::elias_fano_builder builder(edges.back(), edges.size());
  for (auto e : edges)
    builder.push_back(e);
  succinct::elias_fano matrix(&builder);

  std::string fileName = name + ".matrix";
  succinct::mapper::freeze(matrix, fileName.c_str());


  std::cout << "--- Save edge data" << std::endl;
  succinct::gamma_vector edgeVector(edgesData);
  fileName = name + ".edgedata";
  succinct::mapper::freeze(edgeVector, fileName.c_str());

  std::cout << "--- Save edge shortcut id's" << std::endl;
  succinct::gamma_vector edgeIdVector(edgeId);
  fileName = name + ".edgeid";
  succinct::mapper::freeze(edgeIdVector, fileName.c_str());

  std::cout << "--- Save edge shortcuts" << std::endl;
  succinct::bit_vector shortcutsVector(shortcuts);
  fileName = name + ".shortcuts";
  succinct::mapper::freeze(shortcutsVector, fileName.c_str());

  /// @todo Restore this checking. Now data facade depends on mwm libraries.
  /*
  std::cout << "--- Test packed data" << std::endl;
  routing::OsrmDataFacade<QueryEdge::EdgeData> facadeNew(name);

  std::cout << "Check node count " << facade.GetNumberOfNodes() << " == " << facadeNew.GetNumberOfNodes() <<  "...";
  PrintStatus(facade.GetNumberOfNodes() == facadeNew.GetNumberOfNodes());
  std::cout << "Check edges count " << facade.GetNumberOfEdges() << " == " << facadeNew.GetNumberOfEdges() << "...";
  PrintStatus(facade.GetNumberOfEdges() == facadeNew.GetNumberOfEdges());

  std::cout << "Check edges data ...";
  bool error = false;
  assert(facade.GetNumberOfEdges() == facadeNew.GetNumberOfEdges());
  for (uint32_t e = 0; e < facade.GetNumberOfEdges(); ++e)
  {
    QueryEdge::EdgeData d1 = facade.GetEdgeData(e);
    QueryEdge::EdgeData d2 = facadeNew.GetEdgeData(e);

    if (d1.backward != d2.backward ||
        d1.forward != d2.forward ||
        d1.distance != d2.distance ||
        d1.id != d2.id ||
        d1.shortcut != d2.shortcut)
    {
      std::cout << "Edge num: " << e << std::endl;
      std::cout << "d1 (backward: " << (uint32_t)d1.backward << ", forward: " << (uint32_t)d1.forward << ", distance: "
                << (uint32_t)d1.distance << ", id: " << (uint32_t)d1.id << ", shortcut: " << (uint32_t)d1.shortcut << std::endl;
      std::cout << "d2 (backward: " << (uint32_t)d2.backward << ", forward: " << (uint32_t)d2.forward << ", distance: "
                << (uint32_t)d2.distance << ", id: " << (uint32_t)d2.id << ", shortcut: " << (uint32_t)d2.shortcut << std::endl;
      error = true;
      break;
    }
  }
  PrintStatus(!error);

  std::cout << "Check graph structure...";
  error = false;
  for (uint32_t node = 0; node < facade.GetNumberOfNodes(); ++node)
  {
    EdgeRange r1 = facade.GetAdjacentEdgeRange(node);
    EdgeRange r2 = facadeNew.GetAdjacentEdgeRange(node);

    if ((r1.front() != r2.front()) || (r1.back() != r2.back()))
    {
      std::cout << "Node num: " << node << std::endl;
      std::cout << "r1 (" << r1.front() << ", " << r1.back() << ")" << std::endl;
      std::cout << "r2 (" << r2.front() << ", " << r2.back() << ")" << std::endl;

      error = true;
      break;
    }
  }
  PrintStatus(!error);
  */
}

}
