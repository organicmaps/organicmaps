#pragma once

#include "drape/binding_info.hpp"
#include "drape/pointers.hpp"

#include "base/shared_buffer_manager.hpp"

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace dp
{
struct MutateRegion
{
  MutateRegion() : m_offset(0), m_count(0) {}
  MutateRegion(uint32_t offset, uint32_t count) : m_offset(offset), m_count(count) {}

  uint32_t m_offset;  // Offset from buffer begin in "Elements" not in bytes.
  uint32_t m_count;   // Count of "Elements".
};

struct MutateNode
{
  MutateRegion m_region;
  ref_ptr<void> m_data;
};

class AttributeBufferMutator
{
  using TBufferNode = std::pair<SharedBufferManager::shared_buffer_ptr_t, size_t>;
  using TBufferArray = std::vector<TBufferNode>;
  using TMutateNodes = std::vector<MutateNode>;
  using TMutateData = std::map<BindingInfo, TMutateNodes>;

public:
  ~AttributeBufferMutator();
  void AddMutation(BindingInfo const & info, MutateNode const & node);
  void * AllocateMutationBuffer(size_t byteCount);

private:
  friend class VertexArrayBuffer;
  TMutateData const & GetMutateData() const { return m_data; }

  TMutateData m_data;
  TBufferArray m_array;
};
}  // namespace dp
