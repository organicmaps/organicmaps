#include "tesselator.hpp"
#include "geometry_coding.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

#include "../std/queue.hpp"

#include "../../3party/sgitess/interface.h"

#include "../base/start_mem_debug.hpp"


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

  void TesselateInterior(PointsT const & bound, HolesT const & holes, TrianglesInfo & info)
  {
    tess::VectorDispatcher disp;
    tess::Tesselator tess;
    tess.setDispatcher(&disp);
    tess.setWindingRule(tess::WindingOdd);

    tess.beginPolygon();

    tess.beginContour();
    for_each(bound.begin(), bound.end(), AddTessPointF(tess));
    tess.endContour();

    for (HolesT::const_iterator it = holes.begin(); it != holes.end(); ++it)
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
    pair<neighbours_t::iterator, bool> ret = m_neighbours.insert(make_pair(make_pair(p1, p2), trg));

    // triangles should not duplicate
    CHECK ( ret.second, ("Duplicating triangles for indices : ", p1, p2) );
  }

  void TrianglesInfo::ListInfo::Add(uintptr_t const * arr)
  {
    int arr32[] = { arr[0], arr[1], arr[2] };
    m_triangles.push_back(Triangle(arr32));

    size_t const trg = m_triangles.size()-1;
    for (int i = 0; i < 3; ++i)
      AddNeighbour(arr32[i], arr32[(i+1)%3], trg);
  }

  /// Find best (cheap in serialization) start edge for processing.
  TrianglesInfo::ListInfo::iter_t
  TrianglesInfo::ListInfo::FindStartTriangle() const
  {
    for (iter_t i = m_neighbours.begin(); i != m_neighbours.end(); ++i)
    {
      if (m_neighbours.find(make_pair(i->first.second, i->first.first)) == m_neighbours.end())
        return i;
    }

    ASSERT ( false, ("?WTF? There is no border triangles!") );
    return m_neighbours.end();
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

    ASSERT ( false, ("?WTF? Triangles not neighbours!") );
    return make_pair(-1, -1);
  }

  /// Get neighbours of 'trg' triangle, wich was riched from 'from' triangle.
  /// @param[out] nb  Neighbours indexes of 'trg' if 0->1 is common edge with'from':
  /// - nb[0] - by 1->2 edge;
  /// - nb[1] - by 2->0 edge;
  void TrianglesInfo::ListInfo::GetNeighbours(
      Triangle const & trg, Triangle const & from, int * nb) const
  {
    int i = my::NextModN(CommonEdge(trg, from).first, 3);
    int j = my::NextModN(i, 3);

    int ind = 0;
    iter_t it = m_neighbours.find(make_pair(trg.m_p[j], trg.m_p[i]));
    nb[ind++] = (it != m_neighbours.end()) ? it->second : empty_key;

    it = m_neighbours.find(make_pair(trg.m_p[my::NextModN(j, 3)], trg.m_p[j]));
    nb[ind++] = (it != m_neighbours.end()) ? it->second : empty_key;
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

  // Element with less m_delta is better than another one.
  struct edge_greater_delta
  {
    bool operator() (Edge const & e1, Edge const & e2) const
    {
      return (e1.m_delta > e2.m_delta);
    }
  };

  void TrianglesInfo::ListInfo::MakeTrianglesChain(
      PointsInfo const & points, iter_t start, vector<Edge> & chain) const
  {
    Triangle const fictive(start->first.second, start->first.first, -1);

    priority_queue<Edge, vector<Edge>, edge_greater_delta> q;
    q.push(Edge(-1, start->second, 0.0, -1));

    // marks of visited nodes
    vector<bool> visited;
    visited.resize(m_triangles.size());

    while (!q.empty())
    {
      // pop current element
      Edge e = q.top();
      q.pop();

      // check if already processed
      if (visited[e.m_p[1]])
        continue;
      visited[e.m_p[1]] = true;

      // push to chain
      chain.push_back(e);

      Triangle const & trg = m_triangles[e.m_p[1]];

      // get neighbours
      int nb[2];
      GetNeighbours(trg, (e.m_p[0] == -1) ? fictive : m_triangles[e.m_p[0]], nb);

      // push neighbours to queue
      for (int i = 0; i < 2; ++i)
        if (nb[i] != empty_key && !visited[nb[i]])
          q.push(Edge(e.m_p[1], nb[i], CalcDelta(points, trg, m_triangles[nb[i]]), i));
    }
  }

  void TrianglesInfo::Add(uintptr_t const * arr)
  {
    m2::PointD arrP[] = { m_points[arr[0]], m_points[arr[1]], m_points[arr[2]] };
    double const cp = m2::CrossProduct(arrP[1] - arrP[0], arrP[2] - arrP[1]);

    if (fabs(cp) > 1.0E-4)
    {
      bool const isCCW = (cp > 0.0);

      if (m_isCCW == 0)
        m_isCCW = (isCCW ? 1 : -1);
      else
        CHECK_EQUAL ( m_isCCW == 1, isCCW, () );
    }

    m_triangles.back().Add(arr);
  }
}
