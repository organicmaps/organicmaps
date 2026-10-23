#include "testing/testing.hpp"

#include "generator/tesselator.hpp"

#include "3party/libtess2/Include/tesselator.h"

#include "base/logging.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace tesselator_test
{
typedef m2::PointD P;

class DoDump
{
  size_t & m_count;

public:
  explicit DoDump(size_t & count) : m_count(count) { m_count = 0; }
  void operator()(P const & p1, P const & p2, P const & p3)
  {
    ++m_count;
    LOG(LINFO, (p1, p2, p3));
  }
};

size_t RunTest(std::list<std::vector<P>> const & l)
{
  tesselator::TrianglesInfo info;
  int const trianglesCount = tesselator::TesselateInterior(l, info);

  size_t count;
  info.ForEachTriangle(DoDump(count));
  TEST_EQUAL(count, static_cast<size_t>(trianglesCount), ());
  return count;
}

size_t RunTess(P const * arr, size_t count)
{
  std::list<std::vector<P>> l;
  l.emplace_back();
  l.back().assign(arr, arr + count);

  return RunTest(l);
}

UNIT_TEST(Tesselator_SelfISect)
{
  P arr[] = {P(0, 0), P(0, 4), P(4, 4), P(1, 1), P(1, 3), P(4, 0), P(0, 0)};
  TEST_EQUAL(6, RunTess(arr, ARRAY_SIZE(arr)), ());
}

UNIT_TEST(Tesselator_Odd)
{
  P arr[] = {P(-100, -100), P(100, 100), P(100, -100)};

  size_t const count = ARRAY_SIZE(arr);

  std::list<std::vector<P>> l;
  l.emplace_back();
  l.back().assign(arr, arr + count);
  l.emplace_back();
  l.back().assign(arr, arr + count);

  TEST_EQUAL(0, RunTest(l), ());

  P arr1[] = {P(-100, -100), P(100, -100), P(100, 100), P(-100, 100)};

  l.emplace_back();
  l.back().assign(arr1, arr1 + ARRAY_SIZE(arr1));

  TEST_EQUAL(2, RunTest(l), ());
}

namespace
{
using TessPtr = std::unique_ptr<TESStesselator, decltype(&tessDeleteTess)>;
using Contours = tesselator::PolygonsT;

struct Allocations
{
  size_t m_calls = 0;
  size_t m_failAt = 0;
  size_t m_live = 0;

  bool ShouldFail() { return ++m_calls == m_failAt; }

  static void * Allocate(void * userData, unsigned int size)
  {
    auto & state = *static_cast<Allocations *>(userData);
    if (state.ShouldFail())
      return nullptr;
    void * memory = std::malloc(size);
    if (memory)
    {
      ++state.m_live;
      std::memset(memory, 0xa5, size);
    }
    return memory;
  }

  static void * Reallocate(void * userData, void * memory, unsigned int size)
  {
    auto & state = *static_cast<Allocations *>(userData);
    if (state.ShouldFail())
      return nullptr;
    bool const isNew = memory == nullptr;
    void * resized = std::realloc(memory, size);
    if (resized && isNew)
      ++state.m_live;
    return resized;
  }

  static void Free(void * userData, void * memory)
  {
    auto & state = *static_cast<Allocations *>(userData);
    if (memory)
    {
      CHECK_GREATER(state.m_live, 0, ());
      --state.m_live;
      std::free(memory);
    }
  }
};

TESSalloc MakeAllocator(Allocations & allocations)
{
  return {
      .memalloc = &Allocations::Allocate,
      .memrealloc = &Allocations::Reallocate,
      .memfree = &Allocations::Free,
      .userData = &allocations,
      .meshEdgeBucketSize = 16,
      .meshVertexBucketSize = 16,
      .meshFaceBucketSize = 16,
      .dictNodeBucketSize = 16,
      .regionBucketSize = 16,
      .extraVertices = 0,
  };
}

void AddContours(TESStesselator * tess, Contours const & contours)
{
  for (auto const & contour : contours)
    tessAddContour(tess, 2, contour.data(), sizeof(P), static_cast<int>(contour.size()));
}

int Tessellate(TESStesselator * tess, int elementType)
{
  return tessTesselate(tess, TESS_WINDING_ODD, elementType, 3, 2, nullptr);
}

void CheckOutput(TESStesselator * tess, int elementType)
{
  int const vertices = tessGetVertexCount(tess);
  int const elements = tessGetElementCount(tess);
  TEST_GREATER(vertices, 0, ());
  TEST_GREATER(elements, 0, ());
  TEST(tessGetVertices(tess), ());
  TEST(tessGetVertexIndices(tess), ());
  auto const * indices = tessGetElements(tess);
  TEST(indices, ());
  for (int i = 0; i < elements; ++i)
  {
    if (elementType == TESS_BOUNDARY_CONTOURS)
    {
      TEST_GREATER_OR_EQUAL(indices[i * 2], 0, (i));
      TEST_GREATER_OR_EQUAL(indices[i * 2 + 1], 3, (i));
      TEST_LESS_OR_EQUAL(indices[i * 2] + indices[i * 2 + 1], vertices, (i));
    }
    else
    {
      int const stride = elementType == TESS_CONNECTED_POLYGONS ? 6 : 3;
      for (int j = 0; j < 3; ++j)
      {
        TEST_GREATER_OR_EQUAL(indices[i * stride + j], 0, (i, j));
        TEST_LESS(indices[i * stride + j], vertices, (i, j));
        if (elementType == TESS_CONNECTED_POLYGONS)
        {
          auto const neighbour = indices[i * stride + 3 + j];
          TEST(neighbour == TESS_UNDEF || (neighbour >= 0 && neighbour < elements), (i, j));
        }
      }
    }
  }
}

Contours RadialContour(int count, int step = 1)
{
  Contours contours(1);
  for (int i = 0; i < count; ++i)
  {
    double const angle = 2 * math::pi * ((i * step) % count) / count;
    double const radius = i % 2 ? 10 : 5;
    contours.back().emplace_back(radius * std::cos(angle), radius * std::sin(angle));
  }
  return contours;
}

size_t CheckAllocationFailure(Contours const & contours, int elementType, size_t failAt)
{
  Allocations state{.m_failAt = failAt};
  auto alloc = MakeAllocator(state);
  {
    TessPtr tess(tessNewTess(&alloc), &tessDeleteTess);
    if (tess)
    {
      AddContours(tess.get(), contours);
      auto const result = Tessellate(tess.get(), elementType);
      TEST_EQUAL(result, failAt ? 0 : 1, (elementType, failAt));
      if (failAt)
      {
        auto const calls = state.m_calls;
        AddContours(tess.get(), contours);
        auto const retry = Tessellate(tess.get(), elementType);
        TEST_EQUAL(retry, 0, (elementType, failAt));
        TEST_EQUAL(state.m_calls, calls, (elementType, failAt));
      }
      else
        CheckOutput(tess.get(), elementType);
    }
    else
      TEST_NOT_EQUAL(failAt, 0, ());
  }
  TEST_GREATER_OR_EQUAL(state.m_calls, failAt, (elementType));
  TEST_EQUAL(state.m_live, 0, (elementType, failAt));
  return state.m_calls;
}
}  // namespace

UNIT_TEST(Tesselator_AllocationFailures)
{
  Contours const square = {{{0, 0}, {10, 0}, {10, 10}, {0, 10}}};
  Contours const selfIntersection = {{{0, 0}, {0, 4}, {4, 4}, {1, 1}, {1, 3}, {4, 0}, {0, 0}}};
  Contours holes = square;
  for (int i = 0; i < 10; ++i)
  {
    double const x = 1 + i * 0.7;
    holes.push_back({{x, 1}, {x + 0.3, 1}, {x + 0.3, 7}, {x, 7}});
  }

  for (auto const & contours : {square, selfIntersection, holes, RadialContour(31, 13), RadialContour(1000)})
  {
    for (int type : {TESS_POLYGONS, TESS_CONNECTED_POLYGONS, TESS_BOUNDARY_CONTOURS})
    {
      auto const calls = CheckAllocationFailure(contours, type, 0);
      for (size_t failAt = 1; failAt <= calls; ++failAt)
        CheckAllocationFailure(contours, type, failAt);
    }
  }
}

UNIT_TEST(Tesselator_DiscardContours)
{
  auto const contours = RadialContour(1000);
  size_t calls = 0;
  for (size_t failAt = 0; failAt <= calls; ++failAt)
  {
    Allocations state{.m_failAt = failAt};
    auto alloc = MakeAllocator(state);
    {
      TessPtr tess(tessNewTess(&alloc), &tessDeleteTess);
      if (tess)
        AddContours(tess.get(), contours);
      else
        TEST_NOT_EQUAL(failAt, 0, ());
    }
    if (!failAt)
      calls = state.m_calls;
    TEST_GREATER_OR_EQUAL(state.m_calls, failAt, ());
    TEST_EQUAL(state.m_live, 0, (failAt));
  }
}

UNIT_TEST(Tesselator_Reuse)
{
  Contours const square = {{{0, 0}, {10, 0}, {10, 10}, {0, 10}}};
  for (int type : {TESS_POLYGONS, TESS_CONNECTED_POLYGONS, TESS_BOUNDARY_CONTOURS})
  {
    Allocations state;
    auto alloc = MakeAllocator(state);
    {
      TessPtr tess(tessNewTess(&alloc), &tessDeleteTess);
      TEST(tess, ());
      for (int i = 0; i < 2; ++i)
      {
        AddContours(tess.get(), square);
        auto const result = Tessellate(tess.get(), type);
        TEST_EQUAL(result, 1, (type, i));
        CheckOutput(tess.get(), type);
        TEST_EQUAL(tessGetVertexCount(tess.get()), 4, ());
        TEST_EQUAL(tessGetElementCount(tess.get()), type == TESS_BOUNDARY_CONTOURS ? 1 : 2, ());
      }
    }
    TEST_EQUAL(state.m_live, 0, (type));
  }
}
}  // namespace tesselator_test
