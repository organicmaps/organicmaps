#pragma once

#include "pointers.hpp"
#include "binding_info.hpp"

#include "../std/stdint.hpp"
#include "../std/map.hpp"

struct MutateRegion
{
  MutateRegion() : m_offset(0), m_count(0) {}
  MutateRegion(uint16_t offset, uint16_t count) : m_offset(offset), m_count(count) {}

  uint16_t m_offset; // Offset from buffer begin in "Elements" not in bytes
  uint16_t m_count;  // Count of "Elements".
};

struct MutateNode
{
  MutateRegion m_region;
  RefPointer<void> m_data;
};

class AttributeBufferMutator
{
  typedef vector<MutateNode> mutate_nodes_t;
  typedef map<BindingInfo, mutate_nodes_t> mutate_data_t;
public:
  void AddMutation(BindingInfo const & info, MutateNode const & node);

private:
  friend class VertexArrayBuffer;
  mutate_data_t const & GetMutateData() const { return m_data; }

private:
  mutate_data_t m_data;
};
