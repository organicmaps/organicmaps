#include "converter.hpp"

#include <iostream>

#include "../Server/DataStructures/InternalDataFacade.h"

#include "../../../../coding/matrix_traversal.hpp"
#include "../../../../coding/internal/file_data.hpp"
#include "../../../../base/bits.hpp"
#include "../../../../base/logging.hpp"
#include "../../../../routing/osrm_data_facade.hpp"

#include "../../../succinct/elias_fano.hpp"
#include "../../../succinct/elias_fano_compressed_list.hpp"
#include "../../../succinct/gamma_vector.hpp"
#include "../../../succinct/rs_bit_vector.hpp"
#include "../../../succinct/mapper.hpp"


namespace  mapsme
{

void PrintStatus(bool b)
{
  std::cout << (b ? "[Ok]" : "[Fail]") << std::endl;
}

string EdgeDataToString(QueryEdge::EdgeData const & d)
{
  stringstream ss;
  ss << "[" << d.distance <<  ", " << d.shortcut << ", " << d.forward << ", " << d.backward << ", " << d.id << "]";
  return ss.str();
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

  uint64_t const nodeCount = facade.GetNumberOfNodes();

  std::vector<uint64_t> edges;
  std::vector<uint32_t> edgesData;
  std::vector<bool> shortcuts;
  std::vector<uint64_t> edgeId;

  std::cout << "Repack graph..." << std::endl;

  typedef pair<uint64_t, QueryEdge::EdgeData> EdgeInfoT;
  typedef vector<EdgeInfoT> EdgeInfoVecT;

  uint64_t copiedEdges = 0, ignoredEdges = 0;
  for (uint64_t node = 0; node < nodeCount; ++node)
  {
    EdgeInfoVecT edgesInfo;
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
      uint64_t target = facade.GetTarget(edge);
      auto const & data = facade.GetEdgeData(edge);

      edgesInfo.push_back(EdgeInfoT(target, data));
    }

    auto compareFn = [](EdgeInfoT const & a, EdgeInfoT const & b)
    {
      if (a.first != b.first)
        return a.first < b.first;

      if (a.second.forward != b.second.forward)
        return a.second.forward > b.second.forward;

      return a.second.id < b.second.id;
    };

    sort(edgesInfo.begin(), edgesInfo.end(), compareFn);

    uint64_t lastTarget = 0;
    for (auto edge : edgesInfo)
    {
      uint64_t target = edge.first;
      auto const & data = edge.second;

      if (target < lastTarget)
        LOG(LCRITICAL, ("Invalid order of target nodes", target, lastTarget));

      lastTarget = target;
      assert(data.forward || data.backward);

      auto addDataFn = [&](bool b)
      {
        uint64_t e = TraverseMatrixInRowOrder<uint64_t>(nodeCount, node, target, b);

        auto compressId = [&](uint64_t id)
        {
          int id1 = id;
          int id2 = node;

          return bits::ZigZagEncode(id2 - id1);
        };

        if (!edges.empty())
          CHECK_GREATER_OR_EQUAL(e, edges.back(), ());

        if (!edges.empty() && (edges.back() == e))
        {
          LOG(LWARNING, ("Invalid order of edges", e, edges.back(), nodeCount, node, target, b));
          CHECK(data.shortcut == shortcuts.back(), ());

          auto last = edgesData.back();
          if (data.distance <= last)
          {
            if (!edgeId.empty() && data.shortcut)
            {
              CHECK(shortcuts.back(), ());
              auto oldId = node - bits::ZigZagDecode(edgeId.back());

              if (data.distance == last)
                edgeId.back() = compressId(min(oldId, (uint64_t)data.id));
              else
                edgeId.back() = compressId(data.id);
            }

            edgesData.back() = data.distance;
          }

          ++ignoredEdges;
          return;
        }

        edges.push_back(e);
        edgesData.push_back(data.distance);
        shortcuts.push_back(data.shortcut);

        int id1 = data.id;
        int id2 = node;

        if (data.shortcut)
          edgeId.push_back(bits::ZigZagEncode(id2 - id1));
      };

      if (data.forward && data.backward)
      {
        addDataFn(false);
        addDataFn(true);
        copiedEdges++;
      }
      else
        addDataFn(data.backward);
    }
  }

  std::cout << "Edges count: " << edgeId.size() << std::endl;
  PrintStatus(true);

  std::cout << "Sort edges...";
  std::sort(edges.begin(), edges.end());
  PrintStatus(true);

  std::cout << "Edges count: " << edges.size() << std::endl;
  std::cout << "Nodes count: " << nodeCount << std::endl;

  std::cout << "--- Save matrix" << std::endl;
  succinct::elias_fano::elias_fano_builder builder(edges.back(), edges.size());
  for (auto e : edges)
    builder.push_back(e);
  succinct::elias_fano matrix(&builder);

  std::string fileName = name + "." + ROUTING_MATRIX_FILE_TAG;
  succinct::mapper::freeze(matrix, fileName.c_str());


  std::cout << "--- Save edge data" << std::endl;
  succinct::elias_fano_compressed_list edgeVector(edgesData);
  fileName = name + "." + ROUTING_EDGEDATA_FILE_TAG;
  succinct::mapper::freeze(edgeVector, fileName.c_str());

  succinct::elias_fano_compressed_list edgeIdVector(edgeId);
  fileName = name + "." + ROUTING_EDGEID_FILE_TAG;
  succinct::mapper::freeze(edgeIdVector, fileName.c_str());

  std::cout << "--- Save edge shortcuts" << std::endl;
  succinct::rs_bit_vector shortcutsVector(shortcuts);
  fileName = name + "." + ROUTING_SHORTCUTS_FILE_TAG;
  succinct::mapper::freeze(shortcutsVector, fileName.c_str());

  if (edgeId.size() != shortcutsVector.num_ones())
    LOG(LCRITICAL, ("Invalid data"));

  /// @todo Restore this checking. Now data facade depends on mwm libraries.

  std::cout << "--- Test packed data" << std::endl;
  std::string fPath = name + ".mwm";

  {
    FilesContainerW routingCont(fPath);

    auto appendFile = [&] (string const & tag)
    {
      string const fileName = name + "." + tag;
      LOG(LINFO, ("Append file", fileName, "with tag", tag));
      routingCont.Write(fileName, tag);
    };

    appendFile(ROUTING_SHORTCUTS_FILE_TAG);
    appendFile(ROUTING_EDGEDATA_FILE_TAG);
    appendFile(ROUTING_MATRIX_FILE_TAG);
    appendFile(ROUTING_EDGEID_FILE_TAG);

    routingCont.Finish();
  }


  FilesMappingContainer container;
  container.Open(fPath);
  typedef routing::OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT facadeNew;
  facadeNew.Load(container);

  uint64_t edgesCount = facadeNew.GetNumberOfEdges() - copiedEdges + ignoredEdges;
  std::cout << "Check node count " << facade.GetNumberOfNodes() << " == " << facadeNew.GetNumberOfNodes() <<  "...";
  PrintStatus(facade.GetNumberOfNodes() == facadeNew.GetNumberOfNodes());
  std::cout << "Check edges count " << facade.GetNumberOfEdges() << " == " << edgesCount << "...";
  PrintStatus(facade.GetNumberOfEdges() == edgesCount);

  std::cout << "Check edges data ...";
  bool error = false;

  auto errorFn = [&](string const & s)
  {
    my::DeleteFileX(fPath);
    LOG(LCRITICAL, (s));
  };

  typedef vector<QueryEdge::EdgeData> EdgeDataT;
  assert(facade.GetNumberOfEdges() == facadeNew.GetNumberOfEdges());
  for (uint32_t i = 0; i < facade.GetNumberOfNodes(); ++i)
  {
    EdgeDataT v1, v2;

    // get all edges from osrm datafacade and store just minimal weights for duplicates
    typedef pair<NodeID, QueryEdge::EdgeData> EdgeOsrmT;
    vector<EdgeOsrmT> edgesOsrm;
    for (auto e : facade.GetAdjacentEdgeRange(i))
      edgesOsrm.push_back(EdgeOsrmT(facade.GetTarget(e), facade.GetEdgeData(e)));

    auto edgeOsrmLess = [](EdgeOsrmT const & a, EdgeOsrmT const & b)
    {
      if (a.first != b.first)
        return a.first < b.first;

      if (a.second.forward != b.second.forward)
        return a.second.forward < b.second.forward;

      if (a.second.backward != b.second.backward)
        return a.second.backward < b.second.backward;

      if (a.second.distance != b.second.distance)
        return a.second.distance < b.second.distance;

      return a.second.id < b.second.id;
    };
    sort(edgesOsrm.begin(), edgesOsrm.end(), edgeOsrmLess);

    for (size_t k = 1; k < edgesOsrm.size();)
    {
      auto const & e1 = edgesOsrm[k - 1];
      auto const & e2 = edgesOsrm[k];

      if (e1.first != e2.first ||
          e1.second.forward != e2.second.forward ||
          e1.second.backward != e2.second.backward)
      {
        ++k;
        continue;
      }

      if (e1.second.distance > e2.second.distance)
        edgesOsrm.erase(edgesOsrm.begin() + k - 1);
      else
        edgesOsrm.erase(edgesOsrm.begin() + k);
    }

    for (auto e : edgesOsrm)
    {
      QueryEdge::EdgeData d = e.second;
      if (d.forward && d.backward)
      {
        d.backward = false;
        v1.push_back(d);
        d.forward = false;
        d.backward = true;
      }

      v1.push_back(d);
    }

    for (auto e : facadeNew.GetAdjacentEdgeRange(i))
      v2.push_back(facadeNew.GetEdgeData(e, i));

    if (v1.size() != v2.size())
    {
      auto printV = [](EdgeDataT const & v, stringstream & ss)
      {
        for (auto i : v)
          ss << EdgeDataToString(i) << std::endl;
      };

      sort(v1.begin(), v1.end(), EdgeLess());
      sort(v2.begin(), v2.end(), EdgeLess());

      stringstream ss;
      ss << "File name: " << name << std::endl;
      ss << "Not equal edges count for node: " << i << std::endl;
      ss << "v1: " << v1.size() << " v2: " << v2.size() << std::endl;
      ss << "--- v1 ---" << std::endl;
      printV(v1, ss);
      ss << "--- v2 ---" << std::endl;
      printV(v2, ss);

      errorFn(ss.str());
    }

    sort(v1.begin(), v1.end(), EdgeLess());
    sort(v2.begin(), v2.end(), EdgeLess());

    // compare vectors
    for (size_t k = 0; k < v1.size(); ++k)
    {
      QueryEdge::EdgeData const & d1 = v1[k];
      QueryEdge::EdgeData const & d2 = v2[k];

      if (d1.backward != d2.backward ||
          d1.forward != d2.forward ||
          d1.distance != d2.distance ||
          (d1.id != d2.id && (d1.shortcut || d2.shortcut)) ||
          d1.shortcut != d2.shortcut)
      {
        std::cout << "--- " << std::endl;
        for (size_t j = 0; j < v1.size(); ++j)
          std::cout << EdgeDataToString(v1[j]) << " - " << EdgeDataToString(v2[j]) << std::endl;

        stringstream ss;
        ss << "File name: " << name << std::endl;
        ss << "Node: " << i << std::endl;
        ss << EdgeDataToString(d1) << ", " << EdgeDataToString(d2) << std::endl;

        errorFn(ss.str());
      }
    }

  }
  PrintStatus(!error);

  my::DeleteFileX(fPath);
}

}
