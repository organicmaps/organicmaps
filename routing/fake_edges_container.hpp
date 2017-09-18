#pragma once

#include "routing/index_graph_starter.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
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
    CHECK_LESS_OR_EQUAL(m_fake.m_segmentToVertex.size(), numeric_limits<uint32_t>::max(), ());
    return static_cast<uint32_t>(m_fake.m_segmentToVertex.size());
  }

private:
  // finish segment id
  uint32_t m_finishId;
  IndexGraphStarter::FakeGraph m_fake;
};
}  // namespace routing
