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

  size_t GetNumFakeEdges() const { return m_fake.m_segmentToVertex.size(); }

private:
  // finish segment id
  uint32_t m_finishId;
  IndexGraphStarter::FakeGraph m_fake;
};
}  // namespace routing
