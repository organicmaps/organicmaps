#include "geometry_serialization.hpp"
#include "mercator.hpp"
#include "point_to_int64.hpp"
#include "geometry_coding.hpp"

#include "../geometry/pointu_to_uint64.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/stack.hpp"

#include "../base/start_mem_debug.hpp"


namespace serial
{
  namespace pts
  {
    inline m2::PointU D2U(m2::PointD const & p)
    {
      return PointD2PointU(p.x, p.y);
    }

    inline m2::PointD U2D(m2::PointU const & p)
    {
      CoordPointT const pt = PointU2PointD(p);
      return m2::PointD(pt.first, pt.second);
    }

    inline m2::PointU GetMaxPoint()
    {
      return D2U(m2::PointD(MercatorBounds::maxX, MercatorBounds::maxY));
    }

    inline m2::PointU GetBasePoint(int64_t base)
    {
      return m2::Uint64ToPointU(base);
    }
  }

  void Encode(EncodeFunT fn, vector<m2::PointD> const & points, int64_t base, DeltasT & deltas)
  {
    size_t const count = points.size();

    PointsT upoints;
    upoints.reserve(count);

    transform(points.begin(), points.end(), back_inserter(upoints), &pts::D2U);

    ASSERT ( deltas.empty(), () );
    deltas.reserve(count);
    (*fn)(upoints, pts::GetBasePoint(base), pts::GetMaxPoint(), deltas);
  }

  void Decode(DecodeFunT fn, DeltasT const & deltas, int64_t base, OutPointsT & points, size_t reserveF/* = 1*/)
  {
    size_t const count = deltas.size() * reserveF;

    PointsT upoints;
    upoints.reserve(count);

    (*fn)(deltas, pts::GetBasePoint(base), pts::GetMaxPoint(), upoints);

    // It is may be not empty, when storing triangles.
    if (points.empty())
      points.reserve(count);
    transform(upoints.begin(), upoints.end(), back_inserter(points), &pts::U2D);
  }

  void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count, int64_t base, OutPointsT & points)
  {
    DeltasT deltas;
    deltas.reserve(count);
    void const * ret = ReadVarUint64Array(static_cast<char const *>(pBeg), count,
                                          MakeBackInsertFunctor(deltas));

    Decode(fn, deltas, base, points);
    return ret;
  }


  TrianglesChainSaver::TrianglesChainSaver(int64_t base)
  {
    m_base = pts::GetBasePoint(base);
    m_max = pts::GetMaxPoint();
  }

  namespace
  {
    struct edge_less_p0
    {
      typedef tesselator::Edge edge_t;

      bool operator() (edge_t const & e1, edge_t const & e2) const
      {
        return (e1.m_p[0] == e2.m_p[0]) ? (e1.m_side < e2.m_side) : (e1.m_p[0] < e2.m_p[0]);
      }
      bool operator() (edge_t const & e1, int e2) const
      {
        return e1.m_p[0] < e2;
      }
      bool operator() (int e1, edge_t const & e2) const
      {
        return e1 < e2.m_p[0];
      }
    };
  }

  void TrianglesChainSaver::operator() (PointT arr[3], vector<EdgeT> edges)
  {
    m_buffers.push_back(BufferT());
    MemWriter<BufferT> writer(m_buffers.back());

    WriteVarUint(writer, EncodeDelta(arr[0], m_base));
    WriteVarUint(writer, EncodeDelta(arr[1], arr[0]));

    EdgeT curr = edges.front();
    curr.m_delta = EncodeDelta(arr[2], arr[1]);

    sort(edges.begin(), edges.end(), edge_less_p0());

    stack<EdgeT> st;
    while (true)
    {
      CHECK_EQUAL ( curr.m_delta >> 62, 0, () );
      uint64_t delta = curr.m_delta << 2;

      // find next edges
      int const nextNode = curr.m_p[1];
      vector<EdgeT>::iterator i = lower_bound(edges.begin(), edges.end(), nextNode, edge_less_p0());
      bool const found = (i != edges.end() && i->m_p[0] == nextNode);
      if (found)
      {
        // fill 2 tree-struct bites
        ASSERT_NOT_EQUAL(i->m_side, -1, ());

        uint64_t const one = 1;

        // first child
        delta |= (one << i->m_side);

        vector<EdgeT>::iterator j = i+1;
        if (j != edges.end() && j->m_p[0] == nextNode)
        {
          // second child
          ASSERT_EQUAL(i->m_side, 0, ());
          ASSERT_EQUAL(j->m_side, 1, ());

          delta |= (one << j->m_side);

          // push to stack for further processing
          st.push(*j);
        }

        curr = *i;
      }

      // write delta for current element
      WriteVarUint(writer, delta);

      if (!found)
      {
        // end of chain - pop current from stack or exit
        if (st.empty())
          break;
        else
        {
          curr = st.top();
          st.pop();
        }
      }
    }
  }

  void DecodeTriangles(DeltasT const & deltas,
                      m2::PointU const & basePoint,
                      m2::PointU const & maxPoint,
                      PointsT & points)
  {
    size_t const count = deltas.size();
    ASSERT_GREATER ( count, 2, () );

    points.push_back(DecodeDelta(deltas[0], basePoint));
    points.push_back(DecodeDelta(deltas[1], points.back()));
    points.push_back(DecodeDelta(deltas[2] >> 2, points.back()));

    stack<size_t> st;

    size_t ind = 2;
    uint8_t treeBits = deltas[2] & 3;

    for (size_t i = 3; i < count;)
    {
      size_t trg[3];

      if (treeBits & 1)
      {
        // common edge is 1->2
        trg[0] = ind;
        trg[1] = ind-1;
        trg[2] = ind-2;

        // push to stack for further processing
        if (treeBits & 2)
          st.push(ind);
      }
      else if (treeBits & 2)
      {
        // common edge is 2->0
        trg[0] = ind-2;
        trg[1] = ind;
        trg[2] = ind-1;
      }
      else
      {
        // end of chain - pop current from stack
        ASSERT ( !st.empty(), () );
        ind = st.top();
        st.pop();
        treeBits = 2;
        continue;
      }

      // push points
      points.push_back(points[trg[0]]);
      points.push_back(points[trg[1]]);
      points.push_back( DecodeDelta(deltas[i] >> 2,
                        PredictPointInTriangle(maxPoint, points[trg[0]], points[trg[1]], points[trg[2]])));

      // next step
      treeBits = deltas[i] & 3;
      ind = points.size() - 1;
      ++i;
    }

    ASSERT ( treeBits == 0 && st.empty(), () );
  }
}
