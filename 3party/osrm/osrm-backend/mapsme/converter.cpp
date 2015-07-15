#include "converter.hpp"

#include "../server/data_structures/internal_datafacade.hpp"

#include <iostream>

#include "../../../../base/bits.hpp"
#include "../../../../base/logging.hpp"
#include "../../../../base/scope_guard.hpp"

#include "../../../../coding/matrix_traversal.hpp"
#include "../../../../coding/internal/file_data.hpp"

#include "../../../../routing/osrm_data_facade.hpp"

#include "../../../succinct/elias_fano.hpp"
#include "../../../succinct/elias_fano_compressed_list.hpp"
#include "../../../succinct/gamma_vector.hpp"
#include "../../../succinct/rs_bit_vector.hpp"
#include "../../../succinct/mapper.hpp"

namespace  mapsme
{

typedef pair<NodeID, QueryEdge::EdgeData> EdgeOsrmT;

struct EdgeLess
{
  bool operator () (EdgeOsrmT const & e1, EdgeOsrmT const & e2) const
  {
    if (e1.first != e2.first)
      return e1.first < e2.first;

    QueryEdge::EdgeData const & d1 = e1.second;
    QueryEdge::EdgeData const & d2 = e2.second;

    if (d1.distance != d2.distance)
      return d1.distance < d2.distance;

    if (d1.shortcut != d2.shortcut)
      return d1.shortcut < d2.shortcut;

    if (d1.forward != d2.forward)
      return d1.forward < d2.forward;

    if (d1.backward != d2.backward)
      return e1.second.backward < d2.backward;

    if (d1.id != d2.id)
      return d1.id < d2.id;

    return false;
  }
};

void PrintStatus(bool b)
{
  std::cout << (b ? "[Ok]" : "[Fail]") << std::endl;
}

string EdgeDataToString(EdgeOsrmT const & d)
{
  stringstream ss;
  ss << "[" << d.first << ", " << d.second.distance <<  ", " << d.second.shortcut << ", " << d.second.forward << ", "
     << d.second.backward << ", " << d.second.id << "]";
  return ss.str();
}



void GenerateRoutingIndex(const std::string & fPath)
{
  ServerPaths server_paths;

  static const char * kvs[][2] = {{"hsgrdata", ".hsgr"},
                                  {"ramindex", ".ramIndex"},
                                  {"fileindex", ".fileIndex"},
                                  {"geometries", ".geometry"},
                                  {"nodesdata", ".nodes"},
                                  {"edgesdata", ".edges"},
                                  {"namesdata", ".names"},
                                  {"timestamp", ".timestamp"}};

  for (auto const & kv : kvs)
  {
    auto & current_path = server_paths[std::string(kv[0])];
    std::cout << std::string(kv[0]) << " --- " << std::string(kv[1]) << std::endl;
    current_path = boost::filesystem::path(fPath + std::string(kv[1]));
    if (!boost::filesystem::exists(current_path))
    {
      std::cerr << "Can't find file: " << current_path << std::endl;
      exit(1);
    }
  }

  std::cout << "Create internal data facade for file: " << fPath << "...";
  InternalDataFacade<QueryEdge::EdgeData> facade(server_paths);
  PrintStatus(true);

  uint32_t const nodeCount = facade.GetNumberOfNodes();

  std::vector<uint64_t> edges;
  std::vector<uint32_t> edgesData;
  std::vector<bool> shortcuts;
  std::vector<uint64_t> edgeId;

  std::cout << "Repack graph..." << std::endl;

  typedef pair<uint64_t, QueryEdge::EdgeData> EdgeInfoT;
  typedef vector<EdgeInfoT> EdgeInfoVecT;

  uint64_t copiedEdges = 0, ignoredEdges = 0;
  for (uint32_t node = 0; node < nodeCount; ++node)
  {
    EdgeInfoVecT edgesInfo;
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
      uint64_t target = facade.GetTarget(edge);
      auto const & data = facade.GetEdgeData(edge);

      edgesInfo.push_back(EdgeInfoT(target, data));
    }

    sort(edgesInfo.begin(), edgesInfo.end(), [](EdgeInfoT const & a, EdgeInfoT const & b)
    {
      if (a.first != b.first)
        return a.first < b.first;

      if (a.second.forward != b.second.forward)
        return a.second.forward > b.second.forward;

      return a.second.id < b.second.id;
    });

    uint64_t lastTarget = 0;
    for (auto edge : edgesInfo)
    {
      uint64_t const target = edge.first;
      auto const & data = edge.second;

      if (target < lastTarget)
        LOG(LCRITICAL, ("Invalid order of target nodes", target, lastTarget));

      lastTarget = target;
      assert(data.forward || data.backward);

      auto addDataFn = [&](bool b)
      {
        uint64_t const e = TraverseMatrixInRowOrder<uint64_t>(nodeCount, node, target, b);

        auto compressId = [&](uint64_t id)
        {
          return bits::ZigZagEncode(int64_t(node) - int64_t(id));
        };

        if (!edges.empty())
          CHECK_GREATER_OR_EQUAL(e, edges.back(), ());

        if (!edges.empty() && (edges.back() == e))
        {
          LOG(LWARNING, ("Invalid order of edges", e, edges.back(), nodeCount, node, target, b));
          CHECK(data.shortcut == shortcuts.back(), ());

          auto const last = edgesData.back();
          if (static_cast<uint32_t>(data.distance) <= last)
          {
            if (!edgeId.empty() && data.shortcut)
            {
              CHECK(shortcuts.back(), ());
              unsigned oldId = node - bits::ZigZagDecode(edgeId.back());

              if (static_cast<uint32_t>(data.distance) == last)
                edgeId.back() = compressId(min(oldId, data.id));
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

        if (data.shortcut)
          edgeId.push_back(compressId(data.id));
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

  std::string fileName = fPath + "." + ROUTING_MATRIX_FILE_TAG;
  std::ofstream fout(fileName, std::ios::binary);
  fout.write((const char*)&nodeCount, sizeof(nodeCount));
  succinct::mapper::freeze(matrix, fout);
  fout.close();

  std::cout << "--- Save edge data" << std::endl;
  succinct::elias_fano_compressed_list edgeVector(edgesData);
  fileName = fPath + "." + ROUTING_EDGEDATA_FILE_TAG;
  succinct::mapper::freeze(edgeVector, fileName.c_str());

  succinct::elias_fano_compressed_list edgeIdVector(edgeId);
  fileName = fPath + "." + ROUTING_EDGEID_FILE_TAG;
  succinct::mapper::freeze(edgeIdVector, fileName.c_str());

  std::cout << "--- Save edge shortcuts" << std::endl;
  succinct::rs_bit_vector shortcutsVector(shortcuts);
  fileName = fPath + "." + ROUTING_SHORTCUTS_FILE_TAG;
  succinct::mapper::freeze(shortcutsVector, fileName.c_str());

  if (edgeId.size() != shortcutsVector.num_ones())
    LOG(LCRITICAL, ("Invalid data"));

  std::cout << "--- Test packed data" << std::endl;
  std::string path = fPath + ".test";

  {
    FilesContainerW routingCont(path);

    auto appendFile = [&] (string const & tag)
    {
      string const fileName = fPath + "." + tag;
      routingCont.Write(fileName, tag);
    };

    appendFile(ROUTING_SHORTCUTS_FILE_TAG);
    appendFile(ROUTING_EDGEDATA_FILE_TAG);
    appendFile(ROUTING_MATRIX_FILE_TAG);
    appendFile(ROUTING_EDGEID_FILE_TAG);

    routingCont.Finish();
  }

  MY_SCOPE_GUARD(testFileGuard, bind(&my::DeleteFileX, cref(path)));

  FilesMappingContainer container;
  container.Open(path);
  typedef routing::OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT facadeNew;
  facadeNew.Load(container);

  uint64_t edgesCount = facadeNew.GetNumberOfEdges() - copiedEdges + ignoredEdges;
  std::cout << "Check node count " << facade.GetNumberOfNodes() << " == " << facadeNew.GetNumberOfNodes() <<  "..." << std::endl;
  CHECK_EQUAL(facade.GetNumberOfNodes(), facadeNew.GetNumberOfNodes(), ());
  std::cout << "Check edges count " << facade.GetNumberOfEdges() << " == " << edgesCount << "..." << std::endl;
  CHECK_EQUAL(facade.GetNumberOfEdges(), edgesCount, ());

  std::cout << "Check edges data ...";
  bool error = false;

  assert(facade.GetNumberOfEdges() == facadeNew.GetNumberOfEdges());
  for (uint32_t i = 0; i < facade.GetNumberOfNodes(); ++i)
  {
    // get all edges from osrm datafacade and store just minimal weights for duplicates
    vector<EdgeOsrmT> edgesOsrm;
    for (auto e : facade.GetAdjacentEdgeRange(i))
      edgesOsrm.push_back(EdgeOsrmT(facade.GetTarget(e), facade.GetEdgeData(e)));

    sort(edgesOsrm.begin(), edgesOsrm.end(), [](EdgeOsrmT const & a, EdgeOsrmT const & b)
    {
      if (a.first != b.first)
        return a.first < b.first;

      QueryEdge::EdgeData const & d1 = a.second;
      QueryEdge::EdgeData const & d2 = b.second;

      if (d1.forward != d2.forward)
        return d1.forward < d2.forward;

      if (d1.backward != d2.backward)
        return d1.backward < d2.backward;

      if (d1.distance != d2.distance)
        return d1.distance < d2.distance;

      return d1.id < d2.id;
    });

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

    vector<EdgeOsrmT> v1, v2;
    for (auto e : edgesOsrm)
    {
      QueryEdge::EdgeData d = e.second;
      if (d.forward && d.backward)
      {
        d.backward = false;
        v1.push_back(EdgeOsrmT(e.first, d));
        d.forward = false;
        d.backward = true;
      }

      v1.push_back(EdgeOsrmT(e.first, d));
    }

    for (auto e : facadeNew.GetAdjacentEdgeRange(i))
      v2.push_back(EdgeOsrmT(facadeNew.GetTarget(e), facadeNew.GetEdgeData(e, i)));

    if (v1.size() != v2.size())
    {
      auto printV = [](vector<EdgeOsrmT> const & v, stringstream & ss)
      {
        for (auto i : v)
          ss << EdgeDataToString(i) << std::endl;
      };

      sort(v1.begin(), v1.end(), EdgeLess());
      sort(v2.begin(), v2.end(), EdgeLess());

      stringstream ss;
      ss << "File name: " << fPath << std::endl;
      ss << "Not equal edges count for node: " << i << std::endl;
      ss << "v1: " << v1.size() << " v2: " << v2.size() << std::endl;
      ss << "--- v1 ---" << std::endl;
      printV(v1, ss);
      ss << "--- v2 ---" << std::endl;
      printV(v2, ss);

      LOG(LCRITICAL, (ss.str()));
    }

    sort(v1.begin(), v1.end(), EdgeLess());
    sort(v2.begin(), v2.end(), EdgeLess());

    // compare vectors
    for (size_t k = 0; k < v1.size(); ++k)
    {
      EdgeOsrmT const & e1 = v1[k];
      EdgeOsrmT const & e2 = v2[k];

      QueryEdge::EdgeData const & d1 = e1.second;
      QueryEdge::EdgeData const & d2 = e2.second;

      if (e1.first != e2.first ||
          d1.backward != d2.backward ||
          d1.forward != d2.forward ||
          d1.distance != d2.distance ||
          (d1.id != d2.id && (d1.shortcut || d2.shortcut)) ||
          d1.shortcut != d2.shortcut)
      {
        std::cout << "--- " << std::endl;
        for (size_t j = 0; j < v1.size(); ++j)
          std::cout << EdgeDataToString(v1[j]) << " - " << EdgeDataToString(v2[j]) << std::endl;

        LOG(LCRITICAL, ("File:", fPath, "Node:", i, EdgeDataToString(e1), EdgeDataToString(e2)));
      }
    }

  }
  PrintStatus(!error);
}

}
