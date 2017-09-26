#pragma once

#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/segment.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace routing
{
class FakeEdgesContainer final
{
  friend class IndexGraphStarter;

public:
  FakeEdgesContainer(IndexGraphStarter && starter)
    : m_finishId(starter.m_finishId)
    , m_fake(std::move(starter.m_fake))
  {
  }

  uint32_t GetNumFakeEdges() const
  {
    // Maximal number of fake segments in fake graph is numeric_limits<uint32_t>::max()
    // because segment idx type is uint32_t.
    CHECK_LESS_OR_EQUAL(m_fake.GetSize(), std::numeric_limits<uint32_t>::max(), ());
    return static_cast<uint32_t>(m_fake.GetSize());
  }

private:
  // finish segment id
  uint32_t m_finishId;
  FakeGraph<Segment, FakeVertex, Segment> m_fake;
};
}  // namespace routing
