#include "interface.h"
#include "GL/glu.h"
#include <exception>

namespace tess
{
  std::vector<Vertex> const & VectorDispatcher::vertices() const
  {
    return m_vertices;
  }

  std::vector<std::pair<PrimitiveType, std::vector<uintptr_t> > > const & VectorDispatcher::indices() const
  {
    return m_indices;
  }

  void VectorDispatcher::begin(PrimitiveType primType)
  {
    m_first = -1;
    m_second = -1;
    m_indices.push_back(std::pair<PrimitiveType, std::vector<uintptr_t> >(primType, std::vector<uintptr_t>()));
  }

  void VectorDispatcher::end()
  {
  }

  void VectorDispatcher::add(uintptr_t id)
  {
    m_indices.back().second.push_back(id);
  }

  void VectorDispatcher::edge(bool)
  {}

  uintptr_t VectorDispatcher::create(Vertex const & v)
  {
    m_vertices.push_back(v);
    return m_vertices.size() - 1;
  }

  void VectorDispatcher::error(int)
  {}

  Vertex::Vertex(double _x, double _y) : x(_x), y(_y)
  {}

  struct Tesselator::Impl
  {
    Dispatcher * m_disp;
    GLUtesselator * m_tess;

    Impl();
    ~Impl();
  };

  Tesselator::Impl::Impl()
  {
    m_tess = gluNewTess();
  }

  Tesselator::Impl::~Impl()
  {
    gluDeleteTess(m_tess);
  }

  void GLAPIENTRY beginData(GLenum type, GLvoid * userData)
  {
    PrimitiveType primType;

    switch (type)
    {
    case GL_TRIANGLE_FAN   : primType = TrianglesFan; break;
    case GL_TRIANGLES  : primType = TrianglesList; break;
    case GL_TRIANGLE_STRIP : primType = TrianglesStrip; break;
    case GL_LINE_LOOP: primType = LineLoop; break;
    }

    reinterpret_cast<Tesselator::Impl*>(userData)->m_disp->begin(primType);
  }

  void GLAPIENTRY edgeFlagData(GLboolean flag, GLvoid * userData)
  {
    reinterpret_cast<Tesselator::Impl*>(userData)->m_disp->edge(flag != 0);
  }

  void GLAPIENTRY vertexData(GLvoid * vertexData, GLvoid * userData)
  {
    reinterpret_cast<Tesselator::Impl*>(userData)->m_disp->add(reinterpret_cast<uintptr_t>(vertexData));
  }

  void GLAPIENTRY endData(GLvoid * userData)
  {
    reinterpret_cast<Tesselator::Impl*>(userData)->m_disp->end();
  }

  void GLAPIENTRY combineData(GLdouble coords[3], GLvoid * /*vertexData*/[4], GLfloat /*weight*/[4], GLvoid ** outData, GLvoid * userData)
  {
    Tesselator::Impl * impl = reinterpret_cast<Tesselator::Impl*>(userData);
    int id = impl->m_disp->create(Vertex(coords[0], coords[1]));
    *outData = (GLvoid*)id;
  }

  void GLAPIENTRY errorData(GLenum err, GLvoid * userData)
  {
    reinterpret_cast<Tesselator::Impl*>(userData)->m_disp->error(err);
  }

  Tesselator::Tesselator()
  {
    m_impl = new Impl();

    gluTessCallback(m_impl->m_tess, GLU_TESS_BEGIN_DATA, (_GLUfuncptr)beginData);
    gluTessCallback(m_impl->m_tess, GLU_TESS_EDGE_FLAG_DATA, (_GLUfuncptr)edgeFlagData);
    gluTessCallback(m_impl->m_tess, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)vertexData);
    gluTessCallback(m_impl->m_tess, GLU_TESS_END_DATA, (_GLUfuncptr)endData);
    gluTessCallback(m_impl->m_tess, GLU_TESS_COMBINE_DATA, (_GLUfuncptr)combineData);
    gluTessCallback(m_impl->m_tess, GLU_TESS_ERROR_DATA, (_GLUfuncptr)errorData);
  }

  Tesselator::~Tesselator()
  {
    delete m_impl;
  }

  void Tesselator::setDispatcher(Dispatcher * disp)
  {
    m_impl->m_disp = disp;
  }

  void Tesselator::setWindingRule(WindingRule rule)
  {
    double val;

    switch (rule)
    {
    case WindingOdd: val = GLU_TESS_WINDING_ODD; break;
    case WindingNonZero: val = GLU_TESS_WINDING_NONZERO; break;
    case WindingAbsGeqTwo: val = GLU_TESS_WINDING_ABS_GEQ_TWO; break;
    case WindingPositive: val = GLU_TESS_WINDING_POSITIVE; break;
    case WindingNegative: val = GLU_TESS_WINDING_NEGATIVE; break;
    }

    gluTessProperty(m_impl->m_tess, GLU_TESS_WINDING_RULE, val);
  }

  void Tesselator::setTolerance(double val)
  {
    gluTessProperty(m_impl->m_tess, GLU_TESS_TOLERANCE, val);
  }

  void Tesselator::setBoundaryOnly(bool val)
  {
    gluTessProperty(m_impl->m_tess, GLU_TESS_BOUNDARY_ONLY, (double)val);
  }

  void Tesselator::beginPolygon()
  {
    gluTessBeginPolygon(m_impl->m_tess, this->m_impl);
  }

  void Tesselator::endPolygon()
  {
    gluTessEndPolygon(m_impl->m_tess);
  }

  void Tesselator::beginContour()
  {
    gluTessBeginContour(m_impl->m_tess);
  }

  void Tesselator::endContour()
  {
    gluTessEndContour(m_impl->m_tess);
  }

  void Tesselator::add(Vertex const & v)
  {
    int id = m_impl->m_disp->create(v);
    GLdouble coords[3] = {v.x, v.y, 0};
    gluTessVertex(m_impl->m_tess, coords, (GLvoid*)id);
  }
}
