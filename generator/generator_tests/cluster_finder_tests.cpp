#include "testing/testing.hpp"

#include "generator/place_processor.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <list>
#include <string>
#include <vector>

namespace
{
using namespace generator;

enum class Type { T1 = 1, T2, T3 };

std::string DebugPrint(Type const & t)
{
  return "T" + std::to_string(static_cast<int>(t));
}

struct NamedPoint
{
  NamedPoint(m2::PointD const & point, Type const & type, std::string const & name)
    : m_point(point), m_type(type), m_name(name) {}

  size_t m_id = m_counter++;
  m2::PointD m_point;
  Type m_type;
  std::string m_name;

private:
  static size_t m_counter;
};

size_t NamedPoint::m_counter = 0;

m2::RectD GetLimitRect(NamedPoint const & p)
{
   return m2::RectD(p.m_point, p.m_point);
}

bool operator==(NamedPoint const & left, NamedPoint const & right)
{
  return left.m_id == right.m_id;
}

std::string DebugPrint(NamedPoint const & t)
{
  return DebugPrint(t.m_point) + " " + std::to_string(static_cast<int>(t.m_type)) + " " + t.m_name;
}

auto const getRadiusMFunction = [](NamedPoint const & p) {
  switch (p.m_type) {
  case Type::T1: return 4000;
  case Type::T2: return 8000;
  case Type::T3: return 16000;
  };
  UNREACHABLE();
};

auto const isSameFunction = [](NamedPoint const & left, NamedPoint const & right) {
  if (left.m_name != right.m_name)
    return false;

  if (left.m_type != right.m_type)
    return false;

  return MercatorBounds::DistanceOnEarth(left.m_point, right.m_point)  < getRadiusMFunction(left);
};

template <typename T, template<typename, typename> class Container, typename Alloc = std::allocator<T>>
std::vector<std::vector<T>> GetClusters(
    Container<T, Alloc> && container,
    typename ClustersFinder<T, Container, Alloc>::RadiusFunc const & radiusFunc,
    typename ClustersFinder<T, Container, Alloc>::IsSameFunc const & isSameFunc)
{
  return ClustersFinder<T, Container, Alloc>(std::forward<Container<T, Alloc>>(container),
                                             radiusFunc, isSameFunc).Find();
}

UNIT_TEST(ClustersFinder_Empty)
{
  std::list<NamedPoint> emptyList;
  TEST_EQUAL(GetClusters(std::move(emptyList), getRadiusMFunction, isSameFunction),
             std::vector<std::vector<NamedPoint>>(), ());
}

UNIT_TEST(ClustersFinder_OneElement)
{
  NamedPoint p1({0.0, 0.0}, Type::T1, "name");
  std::list<NamedPoint> l{p1};
  std::vector<std::vector<NamedPoint>> expected{{p1}};
  TEST_EQUAL(GetClusters(std::move(l), getRadiusMFunction, isSameFunction), expected, ());
}

UNIT_TEST(ClustersFinder_TwoElements)
{
  NamedPoint p1({0.0, 0.0}, Type::T1, "name");
  NamedPoint p2({0.0001, 0.0001}, Type::T1, "name");
  std::list<NamedPoint> l{p1, p2};

  std::vector<std::vector<NamedPoint>> expected{{p1, p2}};
  TEST_EQUAL(GetClusters(std::move(l), getRadiusMFunction, isSameFunction), expected, ());
}

UNIT_TEST(ClustersFinder_TwoClusters)
{
  {
    NamedPoint p1({0.0, 0.0}, Type::T1, "name1");
    NamedPoint p2({0.0001, 0.0001}, Type::T1, "name2");
    std::list<NamedPoint> l{p1, p2};
    std::vector<std::vector<NamedPoint>> expected{{p2}, {p1}};
    TEST_EQUAL(GetClusters(std::move(l), getRadiusMFunction, isSameFunction), expected, ());
  }
  {
    NamedPoint p1({0.0, 0.0}, Type::T1, "name");
    NamedPoint p2({0.1, 0.1}, Type::T1, "name");
    std::list<NamedPoint> l{p1, p2};

    std::vector<std::vector<NamedPoint>> expected{{p1}, {p2}};
    TEST_EQUAL(GetClusters(std::move(l), getRadiusMFunction, isSameFunction), expected, ());
  }
}

UNIT_TEST(ClustersFinder_ThreeClusters)
{
  NamedPoint p1({0.0, 0.0}, Type::T1, "name");
  NamedPoint p2({0.0, 0.00001}, Type::T1, "name");
  NamedPoint p3({0.0001, 0.0000}, Type::T1, "name");

  NamedPoint p11({0.0, 0.0}, Type::T2, "name");
  NamedPoint p12({0.0, 0.001}, Type::T2, "name");
  NamedPoint p13({0.001, 0.0000}, Type::T2, "name");

  NamedPoint p21({0.0, 0.0}, Type::T1, "name21");
  std::list<NamedPoint> l{p1, p2, p3, p11, p12, p13, p21};
  std::vector<std::vector<NamedPoint>> expected{{p2, p1, p3}, {p11, p13, p12}, {p21}};
  TEST_EQUAL(GetClusters(std::move(l), getRadiusMFunction, isSameFunction), expected, ());
}
}  // namespace
