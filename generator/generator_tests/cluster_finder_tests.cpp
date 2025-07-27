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

namespace cluster_finder_tests
{
enum class Type
{
  T1 = 1,
  T2,
  T3
};

std::string DebugPrint(Type const & t)
{
  return "T" + std::to_string(static_cast<int>(t));
}

struct NamedPoint
{
  NamedPoint(m2::PointD const & point, Type const & type, std::string const & name)
    : m_point(point)
    , m_type(type)
    , m_name(name)
  {}

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

auto const getRadiusMFunction = [](NamedPoint const & p)
{
  switch (p.m_type)
  {
  case Type::T1: return 4000;
  case Type::T2: return 8000;
  case Type::T3: return 16000;
  };
  UNREACHABLE();
};

auto const isSameFunction = [](NamedPoint const & left, NamedPoint const & right)
{ return left.m_name == right.m_name && left.m_type == right.m_type; };

using ClusterT = std::vector<NamedPoint const *>;

void Sort(std::vector<ClusterT> & data)
{
  for (auto & d : data)
    std::sort(std::begin(d), std::end(d), [](NamedPoint const * l, NamedPoint const * r) { return l->m_id < r->m_id; });
  std::sort(std::begin(data), std::end(data), [](ClusterT const & l, ClusterT const & r)
  {
    TEST(!l.empty(), ());
    TEST(!r.empty(), ());
    return l.front()->m_id < r.front()->m_id;
  });
}

void Test(std::vector<NamedPoint> const & input, std::vector<ClusterT> & expected)
{
  auto result = generator::GetClusters(input, getRadiusMFunction, isSameFunction);

  Sort(result);
  Sort(expected);
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(ClustersFinder_Empty)
{
  std::vector<ClusterT> expected;
  Test({}, expected);
}

UNIT_TEST(ClustersFinder_OneElement)
{
  std::vector<NamedPoint> in{NamedPoint({0.0, 0.0}, Type::T1, "name")};
  std::vector<ClusterT> expected{{&in[0]}};
  Test(in, expected);
}

UNIT_TEST(ClustersFinder_TwoElements)
{
  std::vector<NamedPoint> in{NamedPoint({0.0, 0.0}, Type::T1, "name"), NamedPoint({0.0001, 0.0001}, Type::T1, "name")};

  std::vector<ClusterT> expected{{&in[0], &in[1]}};
  Test(in, expected);
}

UNIT_TEST(ClustersFinder_TwoClusters)
{
  {
    std::vector<NamedPoint> in{NamedPoint({0.0, 0.0}, Type::T1, "name1"),
                               NamedPoint({0.0001, 0.0001}, Type::T1, "name2")};

    std::vector<ClusterT> expected{{&in[1]}, {&in[0]}};
    Test(in, expected);
  }
  {
    std::vector<NamedPoint> in{NamedPoint({0.0, 0.0}, Type::T1, "name"), NamedPoint({0.1, 0.1}, Type::T1, "name")};

    std::vector<ClusterT> expected{{&in[0]}, {&in[1]}};
    Test(in, expected);
  }
}

UNIT_TEST(ClustersFinder_ThreeClusters)
{
  std::vector<NamedPoint> in{
      NamedPoint({0.0, 0.0}, Type::T1, "name"),       NamedPoint({0.0, 0.00001}, Type::T1, "name"),
      NamedPoint({0.0001, 0.0000}, Type::T1, "name"),

      NamedPoint({0.0, 0.0}, Type::T2, "name"),       NamedPoint({0.0, 0.001}, Type::T2, "name"),
      NamedPoint({0.001, 0.0000}, Type::T2, "name"),

      NamedPoint({0.0, 0.0}, Type::T1, "name21")};

  std::vector<ClusterT> expected{{&in[1], &in[0], &in[2]}, {&in[3], &in[5], &in[4]}, {&in[6]}};
  Test(in, expected);
}
}  // namespace cluster_finder_tests
