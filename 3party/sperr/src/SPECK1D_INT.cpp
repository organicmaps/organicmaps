#include "SPECK1D_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

template <typename T>
void sperr::SPECK1D_INT<T>::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it =
        std::remove_if(list.begin(), list.end(), [](const auto& s) { return s.get_length() == 0; });
    list.erase(it, list.end());
  }
}

template <typename T>
void sperr::SPECK1D_INT<T>::m_initialize_lists()
{
  const auto total_len = m_dims[0];
  auto num_of_parts = sperr::num_of_partitions(total_len);
  auto num_of_lists = num_of_parts + 1;
  if (m_LIS.size() < num_of_lists)
    m_LIS.resize(num_of_lists);
  std::for_each(m_LIS.begin(), m_LIS.end(), [](auto& list) { list.clear(); });

  // Put in two sets, each representing a half of the long array.
  Set1D set;
  set.set_length(total_len);  // Set represents the whole 1D array.
  auto sets = m_partition_set(set);
  m_LIS[sets[0].get_level()].emplace_back(sets[0]);
  m_LIS[sets[1].get_level()].emplace_back(sets[1]);
}

template <typename T>
auto sperr::SPECK1D_INT<T>::m_partition_set(Set1D set) const -> std::array<Set1D, 2>
{
  const auto start = set.get_start();
  const auto length = set.get_length();
  const auto level = set.get_level();
  std::array<Set1D, 2> subsets;

  // Prepare the 1st set
  auto& set1 = subsets[0];
  set1.set_start(start);
  set1.set_length(length - length / 2);
  set1.set_level(level + 1);
  // Prepare the 2nd set
  auto& set2 = subsets[1];
  set2.set_start(start + length - length / 2);
  set2.set_length(length / 2);
  set2.set_level(level + 1);

  return subsets;
}

template class sperr::SPECK1D_INT<uint64_t>;
template class sperr::SPECK1D_INT<uint32_t>;
template class sperr::SPECK1D_INT<uint16_t>;
template class sperr::SPECK1D_INT<uint8_t>;
