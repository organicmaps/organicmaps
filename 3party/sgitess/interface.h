#pragma once

#include <vector>
#include <utility>  // pair

#ifndef _MSC_VER
#include <stdint.h> // uintptr_t
#endif

namespace tess
{
  enum WindingRule
  {
    WindingOdd,
    WindingNonZero,
    WindingAbsGeqTwo,
    WindingPositive,
    WindingNegative
  };

  enum PrimitiveType
  {
    TrianglesFan,
    TrianglesStrip,
    TrianglesList,
    LineLoop
  };

  struct Vertex
  {
    double x;
    double y;
    Vertex(double _x = 0, double _y = 0);
  };

  struct Dispatcher
  {
    virtual void begin(PrimitiveType type) = 0;
    virtual void end() = 0;
    virtual void add(uintptr_t id) = 0;
    virtual void edge(bool flag) = 0;
    virtual uintptr_t create(Vertex const & v) = 0;
    virtual void error(int err) = 0;
  };

  class VectorDispatcher : public Dispatcher
  {
    std::vector<Vertex> m_vertices;

    std::vector<std::pair<PrimitiveType, std::vector<uintptr_t> > > m_indices;

    int m_first;
    int m_second;

  public:

    void begin(PrimitiveType primType);
    void end();
    void add(uintptr_t id);
    void edge(bool flag);
    uintptr_t create(tess::Vertex const & v);
    void error(int err);

    std::vector<Vertex> const & vertices() const;
    std::vector<std::pair<PrimitiveType, std::vector<uintptr_t> > > const & indices() const;
  };

  struct Tesselator
  {
  public:
    struct Impl;
  private:

    Impl * m_impl;

  public:

    Tesselator();
    ~Tesselator();

    void setDispatcher(Dispatcher * disp);
    void setWindingRule(WindingRule rule);
    void setTolerance(double val);
    void setBoundaryOnly(bool val);

    void beginPolygon();
    void beginContour();
    void add(Vertex const & v);
    void endContour();
    void endPolygon();
  };
}
