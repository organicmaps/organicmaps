#include "testing/testing.hpp"

#include "generator/cluster_finder.hpp"

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
  return DebugPrint(t.m_point) + " " + DebugPrint(t.m_type) + " " + t.m_name;
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
  return left.m_name == right.m_name && left.m_type == right.m_type;
};

void Sort(std::vector<std::vector<NamedPoint>> & data)
{
  for (auto & d : data)
  {
    std::sort(std::begin(d), std::end(d), [](auto const & l, auto const & r) {
      return l.m_id < r.m_id;
    });
  }
  std::sort(std::begin(data), std::end(data), [](auto const & l, auto const & r) {
    CHECK(!l.empty(), ());
    CHECK(!r.empty(), ());
    return l.front().m_id < r.front().m_id;
  });
}

void Test(std::vector<std::vector<NamedPoint>> & result, std::vector<std::vector<NamedPoint>> & expected)
{
  Sort(result);
  Sort(expected);
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(ClustersFinder_Empty)
{
  std::list<NamedPoint> emptyList;
  std::vector<std::vector<NamedPoint>> expected;
  auto result = GetClusters(std::move(emptyList), getRadiusMFunction, isSameFunction);
  Test(result, expected);
}

UNIT_TEST(ClustersFinder_OneElement)
{
  NamedPoint p1({0.0, 0.0}, Type::T1, "name");
  std::list<NamedPoint> l{p1};
  std::vector<std::vector<NamedPoint>> expected{{p1}};
  auto result = GetClusters(std::move(l), getRadiusMFunction, isSameFunction);
  Test(result, expected);
}

UNIT_TEST(ClustersFinder_TwoElements)
{
  NamedPoint p1({0.0, 0.0}, Type::T1, "name");
  NamedPoint p2({0.0001, 0.0001}, Type::T1, "name");
  std::list<NamedPoint> l{p1, p2};

  std::vector<std::vector<NamedPoint>> expected{{p1, p2}};
  auto result = GetClusters(std::move(l), getRadiusMFunction, isSameFunction);
  Test(result, expected);
}

UNIT_TEST(ClustersFinder_TwoClusters)
{
  {
    NamedPoint p1({0.0, 0.0}, Type::T1, "name1");
    NamedPoint p2({0.0001, 0.0001}, Type::T1, "name2");
    std::list<NamedPoint> l{p1, p2};
    std::vector<std::vector<NamedPoint>> expected{{p2}, {p1}};
    auto result = GetClusters(std::move(l), getRadiusMFunction, isSameFunction);
    Test(result, expected);
  }
  {
    NamedPoint p1({0.0, 0.0}, Type::T1, "name");
    NamedPoint p2({0.1, 0.1}, Type::T1, "name");
    std::list<NamedPoint> l{p1, p2};

    std::vector<std::vector<NamedPoint>> expected{{p1}, {p2}};
    auto result = GetClusters(std::move(l), getRadiusMFunction, isSameFunction);
    Test(result, expected);
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
  auto result = GetClusters(std::move(l), getRadiusMFunction, isSameFunction);
  Test(result, expected);
}
}  // namespace
