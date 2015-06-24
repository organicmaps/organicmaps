#include "generator/tesselator.hpp"

#include "indexer/geometry_coding.hpp"

#include "geometry/robust_orientation.hpp"

#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/queue.hpp"

#include "3party/sgitess/interface.h"


namespace tesselator
{
  struct AddTessPointF
  {
    tess::Tesselator & m_tess;
    AddTessPointF(tess::Tesselator & tess) : m_tess(tess)
    {}
    void operator()(m2::PointD const & p)
    {
      m_tess.add(tess::Vertex(p.x, p.y));
    }
  };

  void TesselateInterior(PolygonsT const & polys, TrianglesInfo & info)
  {
    tess::VectorDispatcher disp;
    tess::Tesselator tess;
    tess.setDispatcher(&disp);
    tess.setWindingRule(tess::WindingOdd);

    tess.beginPolygon();

    for (PolygonsT::const_iterator it = polys.begin(); it != polys.end(); ++it)
    {
      tess.beginContour();
      for_each(it->begin(), it->end(), AddTessPointF(tess));
      tess.endContour();
    }

    tess.endPolygon();

    // assign points
    vector<tess::Vertex> const & vert = disp.vertices();
    info.AssignPoints(vert.begin(), vert.end());

    for (size_t i = 0; i < disp.indices().size(); ++i)
    {
      if (disp.indices()[i].first != tess::TrianglesList)
      {
        LOG(LERROR, ("We've got invalid type during teselation:", disp.indices()[i].first));
        continue;
      }

      vector<uintptr_t> const & indices = disp.indices()[i].second;
      size_t const count = indices.size();
      ASSERT_GREATER(count, 0, ());
      ASSERT_EQUAL(count % 3, 0, ());

      info.Reserve(count / 3);
      for (size_t j = 0; j < count; j += 3)
      {
        ASSERT_LESS ( j+2, count, () );
        info.Add(&indices[j]);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  // TrianglesInfo::ListInfo implementation
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

  int TrianglesInfo::ListInfo::empty_key = -1;

  void TrianglesInfo::ListInfo::AddNeighbour(int p1, int p2, int trg)
  {
    // find or insert element for key
    pair<TNeighbours::iterator, bool> ret = m_neighbors.insert(make_pair(make_pair(p1, p2), trg));

    // triangles should not duplicate
    CHECK ( ret.second, ("Duplicating triangles for indices : ", p1, p2) );
  }

  void TrianglesInfo::ListInfo::Add(uintptr_t const * arr)
  {
    ASSERT_EQUAL(arr[0], static_cast<int>(arr[0]), ());
    ASSERT_EQUAL(arr[1], static_cast<int>(arr[1]), ());
    ASSERT_EQUAL(arr[2], static_cast<int>(arr[2]), ());
    int const arr32[] = { static_cast<int>(arr[0]), static_cast<int>(arr[1]), static_cast<int>(arr[2]) };
    m_triangles.push_back(Triangle(arr32));

    size_t const trg = m_triangles.size()-1;
    for (int i = 0; i < 3; ++i)
      AddNeighbour(arr32[i], arr32[(i+1)%3], trg);
  }

  template <class IterT> size_t GetBufferSize(IterT b, IterT e)
  {
    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    while (b != e) WriteVarUint(writer, *b++);
    return buffer.size();
  }

  /// Find best (cheap in serialization) start edge for processing.
  TrianglesInfo::ListInfo::iter_t
  TrianglesInfo::ListInfo::FindStartTriangle(PointsInfo const & points) const
  {
    iter_t ret = m_neighbors.end();
    size_t cr = numeric_limits<size_t>::max();

    for (iter_t i = m_neighbors.begin(); i != m_neighbors.end(); ++i)
    {
      if (!m_visited[i->second] &&
          m_neighbors.find(make_pair(i->first.second, i->first.first)) == m_neighbors.end())
      {
        uint64_t deltas[3];
        deltas[0] = EncodeDelta(points.m_points[i->first.first], points.m_base);
        deltas[1] = EncodeDelta(points.m_points[i->first.second], points.m_points[i->first.first]);
        deltas[2] = EncodeDelta(points.m_points[m_triangles[i->second].GetPoint3(i->first)],
                                points.m_points[i->first.second]);

        size_t const sz = GetBufferSize(deltas, deltas + 3);
        if (sz < cr)
        {
          ret = i;
          cr = sz;
        }
      }
    }

    ASSERT ( ret != m_neighbors.end(), ("?WTF? There is no border triangles!") );
    return ret;
  }

  /// Return indexes of common edges of [to, from] triangles.
  pair<int, int> CommonEdge(Triangle const & to, Triangle const & from)
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        if (to.m_p[i] == from.m_p[my::NextModN(j, 3)] && to.m_p[my::NextModN(i, 3)] == from.m_p[j])
          return make_pair(i, j);
      }
    }

    ASSERT ( false, ("?WTF? Triangles not neighbors!") );
    return make_pair(-1, -1);
  }

  /// Get neighbors of 'trg' triangle, which was achieved from 'from' triangle.
  /// @param[out] nb  neighbors indexes of 'trg' if 0->1 is common edge with'from':
  /// - nb[0] - by 1->2 edge;
  /// - nb[1] - by 2->0 edge;
  void TrianglesInfo::ListInfo::GetNeighbors(
      Triangle const & trg, Triangle const & from, int * nb) const
  {
    int i = my::NextModN(CommonEdge(trg, from).first, 3);
    int j = my::NextModN(i, 3);

    int ind = 0;
    iter_t it = m_neighbors.find(make_pair(trg.m_p[j], trg.m_p[i]));
    nb[ind++] = (it != m_neighbors.end()) ? it->second : empty_key;

    it = m_neighbors.find(make_pair(trg.m_p[my::NextModN(j, 3)], trg.m_p[j]));
    nb[ind++] = (it != m_neighbors.end()) ? it->second : empty_key;
  }

  /// Calc delta of 'from'->'to' graph edge.
  uint64_t TrianglesInfo::ListInfo::CalcDelta(
      PointsInfo const & points, Triangle const & from, Triangle const & to) const
  {
    pair<int, int> const p = CommonEdge(to, from);

    m2::PointU const prediction =
        PredictPointInTriangle(points.m_max,
                               // common edge with 'to'
                               points.m_points[from.m_p[(p.second+1) % 3]],
                               points.m_points[from.m_p[(p.second)]],
                               // diagonal point of 'from'
                               points.m_points[from.m_p[(p.second+2) % 3]]);

    // delta from prediction to diagonal point of 'to'
    return EncodeDelta(points.m_points[to.m_p[(p.first+2) % 3]], prediction);
  }

  template <class TPopOrder>
  void TrianglesInfo::ListInfo::MakeTrianglesChainImpl(
      PointsInfo const & points, iter_t start, vector<Edge> & chain) const
  {
    chain.clear();

    Triangle const fictive(start->first.second, start->first.first, -1);

    priority_queue<Edge, vector<Edge>, TPopOrder> q;
    q.push(Edge(-1, start->second, 0, -1));

    while (!q.empty())
    {
      // pop current element
      Edge e = q.top();
      q.pop();

      // check if already processed
      if (m_visited[e.m_p[1]])
        continue;
      m_visited[e.m_p[1]] = true;

      // push to chain
      chain.push_back(e);

      Triangle const & trg = m_triangles[e.m_p[1]];

      // get neighbors
      int nb[2];
      GetNeighbors(trg, (e.m_p[0] == -1) ? fictive : m_triangles[e.m_p[0]], nb);

      // push neighbors to queue
      for (int i = 0; i < 2; ++i)
        if (nb[i] != empty_key && !m_visited[nb[i]])
          q.push(Edge(e.m_p[1], nb[i], CalcDelta(points, trg, m_triangles[nb[i]]), i));
    }
  }

  // Element with less m_delta is better than another one.
  struct edge_greater_delta
  {
    bool operator() (Edge const & e1, Edge const & e2) const
    {
      return (e1.m_delta > e2.m_delta);
    }
  };

  // Experimental ...
  struct edge_less_delta
  {
    bool operator() (Edge const & e1, Edge const & e2) const
    {
      return (e1.m_delta < e2.m_delta);
    }
  };

  void TrianglesInfo::ListInfo::MakeTrianglesChain(
    PointsInfo const & points, iter_t start, vector<Edge> & chain, bool /*goodOrder*/) const
  {
    //if (goodOrder)
      MakeTrianglesChainImpl<edge_greater_delta>(points, start, chain);
    //else
    //  MakeTrianglesChainImpl<edge_less_delta>(points, start, chain);
  }

  void TrianglesInfo::Add(uintptr_t const * arr)
  {
    // This checks are useless. We don't care about triangle orientation.

    // When adding triangles, check that they all have identical orientation!
    /*
    m2::PointD arrP[] = { m_points[arr[0]], m_points[arr[1]], m_points[arr[2]] };
    double const cp = m2::robust::OrientedS(arrP[0], arrP[1], arrP[2]);

    if (cp != 0.0)
    {
      bool const isCCW = (cp > 0.0);

      if (m_isCCW == 0)
        m_isCCW = (isCCW ? 1 : -1);
      else
        CHECK_EQUAL ( m_isCCW == 1, isCCW, () );

      m_triangles.back().Add(arr);
    }
    */

    m_triangles.back().Add(arr);
  }

  void TrianglesInfo::GetPointsInfo(m2::PointU const & baseP,
                                    m2::PointU const & maxP,
                                    function<m2::PointU (m2::PointD)> const & convert,
                                    PointsInfo & info) const
  {
    info.m_base = baseP;
    info.m_max = maxP;

    size_t const count = m_points.size();
    info.m_points.reserve(count);
    for (size_t i = 0; i < count; ++i)
      info.m_points.push_back(convert(m_points[i]));
  }
}
