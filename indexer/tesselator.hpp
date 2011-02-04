#pragma once
#include "tesselator_decl.hpp"

#include "../geometry/point2d.hpp"

#include "../std/list.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/iterator.hpp"


namespace tesselator
{
  typedef vector<m2::PointD> PointsT;
  typedef list<PointsT> HolesT;

  struct Triangle
  {
    int m_p[3];

    Triangle(int p0, int p1, int p2)
    {
      m_p[0] = p0;
      m_p[1] = p1;
      m_p[2] = p2;
    }

    Triangle(int const * p)
    {
      for (int i = 0; i < 3; ++i)
        m_p[i] = p[i];
    }

    int GetPoint3(pair<int, int> const & p) const
    {
      for (int i = 0; i < 3; ++i)
        if (m_p[i] != p.first && m_p[i] != p.second)
          return m_p[i];

      ASSERT ( false, ("Triangle with equal points") );
      return -1;
    }
  };

  // Converted points, prepared for serialization.
  struct PointsInfo
  {
    typedef m2::PointU PointT;
    vector<PointT> m_points;
    PointT m_base, m_max;

    PointsInfo(PointT const & baseP, PointT const & maxP)
      : m_base(baseP), m_max(maxP)
    {
    }
  };

  class TrianglesInfo
  {
    PointsT m_points;

    class ListInfo
    {
      static int empty_key;

      vector<Triangle> m_triangles;

      // directed edge -> triangle
      typedef unordered_map<pair<int, int>, int> neighbours_t;
      neighbours_t m_neighbours;

      void AddNeighbour(int p1, int p2, int trg);

      void GetNeighbours(
          Triangle const & trg, Triangle const & from, int * nb) const;

      uint64_t CalcDelta(
          PointsInfo const & points, Triangle const & from, Triangle const & to) const;

    public:
      typedef neighbours_t::const_iterator iter_t;

      ListInfo(size_t count)
      {
        m_triangles.reserve(count);
      }

      void Add(uintptr_t const * arr);

      iter_t FindStartTriangle() const;

      void MakeTrianglesChain(PointsInfo const & points, iter_t start, vector<Edge> & chain) const;

      Triangle GetTriangle(int i) const { return m_triangles[i]; }
    };

    list<ListInfo> m_triangles;

    int m_isCCW;  // 0 - uninitialized; -1 - false; 1 - true

  public:
    TrianglesInfo() : m_isCCW(0) {}

    /// @name Making functions.
    //@{
    template <class IterT> void AssignPoints(IterT b, IterT e)
    {
      m_points.reserve(distance(b, e));
      while (b != e)
      {
        m_points.push_back(m2::PointD(b->x, b->y));
        ++b;
      }
    }

    void Reserve(size_t count) { m_triangles.push_back(ListInfo(count)); }

    void Add(uintptr_t const * arr);
    //@{

    /// Triangles chains processing function.
    template <class EmitterT>
    void ProcessPortions(m2::PointU const & baseP, m2::PointU const & maxP,
                         m2::PointU (*convert) (m2::PointD const &), EmitterT & emitter)
    {
      // convert points from double to uint
      size_t const count = m_points.size();
      PointsInfo points(baseP, maxP);
      points.m_points.reserve(count);
      for (size_t i = 0; i < count; ++i)
        points.m_points.push_back((*convert)(m_points[i]));

      // process portions and push out result chains
      vector<Edge> chain;
      for (list<ListInfo>::const_iterator i = m_triangles.begin(); i != m_triangles.end(); ++i)
      {
        chain.clear();
        typename ListInfo::iter_t start = i->FindStartTriangle();
        i->MakeTrianglesChain(points, start, chain);

        Triangle const trg = i->GetTriangle(start->second);
        m2::PointU arr[] = { points.m_points[start->first.first],
                             points.m_points[start->first.second],
                             points.m_points[trg.GetPoint3(start->first)] };

        emitter(arr, chain);
      }
    }
  };

  /// Main tesselate function.
  void TesselateInterior(PointsT const & bound, HolesT const & holes, TrianglesInfo & info);
}
