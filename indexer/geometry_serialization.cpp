#include "indexer/geometry_serialization.hpp"

#include "indexer/geometry_coding.hpp"

#include "coding/pointd_to_pointu.hpp"

#include "geometry/mercator.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/iterator.hpp"
#include "std/stack.hpp"


namespace serial
{
  namespace pts
  {
    inline m2::PointU D2U(m2::PointD const & p, uint32_t coordBits)
    {
      return PointDToPointU(p, coordBits);
    }

    inline m2::PointD U2D(m2::PointU const & p, uint32_t coordBits)
    {
      m2::PointD const pt = PointUToPointD(p, coordBits);
      ASSERT(MercatorBounds::minX <= pt.x && pt.y <= MercatorBounds::maxX,
             (p, pt, coordBits));
      ASSERT(MercatorBounds::minY <= pt.x && pt.y <= MercatorBounds::maxY,
             (p, pt, coordBits));
      return pt;
    }

    inline m2::PointU GetMaxPoint(CodingParams const & params)
    {
      return D2U(m2::PointD(MercatorBounds::maxX, MercatorBounds::maxY), params.GetCoordBits());
    }

    inline m2::PointU GetBasePoint(CodingParams const & params)
    {
      return params.GetBasePoint();
    }

    typedef buffer_vector<m2::PointU, 32> upoints_t;
  }

  void Encode(EncodeFunT fn, vector<m2::PointD> const & points,
              CodingParams const & params, DeltasT & deltas)
  {
    size_t const count = points.size();

    pts::upoints_t upoints;
    upoints.reserve(count);

    transform(points.begin(), points.end(), back_inserter(upoints),
              bind(&pts::D2U, _1, params.GetCoordBits()));

    ASSERT ( deltas.empty(), () );
    deltas.resize(count);

    geo_coding::OutDeltasT adapt(deltas);
    (*fn)(make_read_adapter(upoints), pts::GetBasePoint(params), pts::GetMaxPoint(params), adapt);
  }

  template <class TDecodeFun, class TOutPoints>
  void DecodeImpl(TDecodeFun fn, DeltasT const & deltas, CodingParams const & params,
                  TOutPoints & points, size_t reserveF)
  {
    size_t const count = deltas.size() * reserveF;

    pts::upoints_t upoints;
    upoints.resize(count);

    geo_coding::OutPointsT adapt(upoints);
    (*fn)(make_read_adapter(deltas), pts::GetBasePoint(params), pts::GetMaxPoint(params), adapt);

    if (points.size() < 2)
    {
      // Do not call reserve when loading triangles - they are accumulated to one vector.
      points.reserve(count);
    }

    transform(upoints.begin(), upoints.begin() + adapt.size(), back_inserter(points),
              bind(&pts::U2D, _1, params.GetCoordBits()));
  }

  void Decode(DecodeFunT fn, DeltasT const & deltas, CodingParams const & params,
              OutPointsT & points, size_t reserveF)
  {
    DecodeImpl(fn, deltas, params, points, reserveF);
  }

  void Decode(DecodeFunT fn, DeltasT const & deltas, CodingParams const & params,
              vector<m2::PointD> & points, size_t reserveF)
  {
    DecodeImpl(fn, deltas, params, points, reserveF);
  }

  void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count,
                         CodingParams const & params, OutPointsT & points)
  {
    DeltasT deltas;
    deltas.reserve(count);
    void const * ret = ReadVarUint64Array(static_cast<char const *>(pBeg), count,
                                          MakeBackInsertFunctor(deltas));

    Decode(fn, deltas, params, points);
    return ret;
  }


  TrianglesChainSaver::TrianglesChainSaver(CodingParams const & params)
  {
    m_base = pts::GetBasePoint(params);
    m_max = pts::GetMaxPoint(params);
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

  void TrianglesChainSaver::operator() (TPoint arr[3], vector<TEdge> edges)
  {
    m_buffers.push_back(TBuffer());
    MemWriter<TBuffer> writer(m_buffers.back());

    WriteVarUint(writer, EncodeDelta(arr[0], m_base));
    WriteVarUint(writer, EncodeDelta(arr[1], arr[0]));

    TEdge curr = edges.front();
    curr.m_delta = EncodeDelta(arr[2], arr[1]);

    sort(edges.begin(), edges.end(), edge_less_p0());

    stack<TEdge> st;
    while (true)
    {
      CHECK_EQUAL ( curr.m_delta >> 62, 0, () );
      uint64_t delta = curr.m_delta << 2;

      // find next edges
      int const nextNode = curr.m_p[1];
      auto i = lower_bound(edges.begin(), edges.end(), nextNode, edge_less_p0());
      bool const found = (i != edges.end() && i->m_p[0] == nextNode);
      if (found)
      {
        // fill 2 tree-struct bites
        ASSERT_NOT_EQUAL(i->m_side, -1, ());

        uint64_t const one = 1;

        // first child
        delta |= (one << i->m_side);

        vector<TEdge>::iterator j = i+1;
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

  void DecodeTriangles(geo_coding::InDeltasT const & deltas,
                       m2::PointU const & basePoint,
                       m2::PointU const & maxPoint,
                       geo_coding::OutPointsT & points)
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
      // points 0, 1 - is a common edge
      // point 2 - is an opposite point for new triangle to calculate prediction
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
      points.push_back(DecodeDelta(deltas[i] >> 2,
                                   PredictPointInTriangle(maxPoint,
                                                          points[trg[0]],
                                                          points[trg[1]],
                                                          points[trg[2]])));

      // next step
      treeBits = deltas[i] & 3;
      ind = points.size() - 1;
      ++i;
    }

    ASSERT ( treeBits == 0 && st.empty(), () );
  }
}
