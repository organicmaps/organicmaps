#include "SPECK2D_INT.h"

#include <algorithm>
#include <cassert>

#if __cplusplus >= 202002L
#include <bit>
#endif

template <typename T>
void sperr::SPECK2D_INT<T>::m_sorting_pass()
{
  // First, process all insignificant pixels.
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    auto value = m_LIP_mask.rlong(i);

#if __cplusplus >= 202002L
    while (value) {
      size_t j = std::countr_zero(value);
      m_process_P(i + j, j, true);
      value &= value - 1;
    }
#else
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          size_t dummy = 0;
          m_process_P(i + j, dummy, true);
        }
      }
    }
#endif
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.rbit(i)) {
      size_t dummy = 0;
      m_process_P(i, dummy, true);
    }
  }

  // Second, process all TypeS sets.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }

  // Third, process the sole TypeI set.
  //
  m_process_I(true);
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto set = m_LIS[idx1][idx2];
  auto subsets = m_partition_S(set);
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](auto s) { return s.is_empty(); });

  auto counter = size_t{0};
  for (auto it = subsets.begin(); it != set_end; ++it) {
    auto need_decide = (counter != 0) || (it + 1 != set_end);
    if (it->is_pixel()) {
      auto pixel_idx = it->start_y * m_dims[0] + it->start_x;
      m_LIP_mask.wtrue(pixel_idx);
      m_process_P(pixel_idx, counter, need_decide);
    }
    else {
      auto newidx1 = it->part_level;
      m_LIS[newidx1].push_back(*it);
      m_process_S(newidx1, m_LIS[newidx1].size() - 1, counter, need_decide);
    }
  }
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_code_I()
{
  auto subsets = m_partition_I();

  auto counter = size_t{0};
  for (auto& set : subsets) {
    if (!set.is_empty()) {
      auto newidx1 = set.part_level;
      m_LIS[newidx1].push_back(set);
      m_process_S(newidx1, m_LIS[newidx1].size() - 1, counter, true);
    }
  }
  m_process_I(counter != 0);
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it = std::remove_if(list.begin(), list.end(), [](auto& s) { return s.is_empty(); });
    list.erase(it, list.end());
  }
}

template <typename T>
auto sperr::SPECK2D_INT<T>::m_partition_S(Set2D set) const -> std::array<Set2D, 4>
{
  auto subsets = std::array<Set2D, 4>();

  const auto detail_len_x = set.length_x / 2;
  const auto detail_len_y = set.length_y / 2;
  const auto approx_len_x = set.length_x - detail_len_x;
  const auto approx_len_y = set.length_y - detail_len_y;

  // Put generated subsets in the list the same order as they are in QccPack.
  auto& BR = subsets[0];  // Bottom right set
  BR.start_x = set.start_x + approx_len_x;
  BR.start_y = set.start_y + approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;
  BR.part_level = set.part_level + 1;

  auto& BL = subsets[1];  // Bottom left set
  BL.start_x = set.start_x;
  BL.start_y = set.start_y + approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;
  BL.part_level = set.part_level + 1;

  auto& TR = subsets[2];  // Top right set
  TR.start_x = set.start_x + approx_len_x;
  TR.start_y = set.start_y;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;
  TR.part_level = set.part_level + 1;

  auto& TL = subsets[3];  // Top left set
  TL.start_x = set.start_x;
  TL.start_y = set.start_y;
  TL.length_x = approx_len_x;
  TL.length_y = approx_len_y;
  TL.part_level = set.part_level + 1;

  return subsets;
}

template <typename T>
auto sperr::SPECK2D_INT<T>::m_partition_I() -> std::array<Set2D, 3>
{
  auto subsets = std::array<Set2D, 3>();
  auto [approx_len_x, detail_len_x] = sperr::calc_approx_detail_len(m_dims[0], m_I.part_level);
  auto [approx_len_y, detail_len_y] = sperr::calc_approx_detail_len(m_dims[1], m_I.part_level);

  // Specify the subsets following the same order in QccPack
  auto& BR = subsets[0];  // Bottom right
  BR.start_x = approx_len_x;
  BR.start_y = approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;
  BR.part_level = m_I.part_level;

  auto& TR = subsets[1];  // Top right
  TR.start_x = approx_len_x;
  TR.start_y = 0;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;
  TR.part_level = m_I.part_level;

  auto& BL = subsets[2];  // Bottom left
  BL.start_x = 0;
  BL.start_y = approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;
  BL.part_level = m_I.part_level;

  // Also update m_I
  m_I.start_x += detail_len_x;
  m_I.start_y += detail_len_y;
  m_I.part_level--;

  return subsets;
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_initialize_lists()
{
  // prepare m_LIS
  auto num_of_parts = sperr::num_of_partitions(std::max(m_dims[0], m_dims[1])) + 1ul;
  if (m_LIS.size() < num_of_parts)
    m_LIS.resize(num_of_parts);
  for (auto& list : m_LIS)
    list.clear();

  // Prepare the root (S), which is the smallest set after multiple levels of transforms.
  // Note that `num_of_xforms` isn't the same as `num_of_parts`.
  //
  auto num_of_xforms = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto [approx_x, detail_x] = sperr::calc_approx_detail_len(m_dims[0], num_of_xforms);
  auto [approx_y, detail_y] = sperr::calc_approx_detail_len(m_dims[1], num_of_xforms);
  auto root = Set2D();
  root.length_x = approx_x;
  root.length_y = approx_y;
  root.part_level = num_of_xforms;
  m_LIS[num_of_xforms].push_back(root);

  // prepare m_I
  m_I.start_x = root.length_x;
  m_I.start_y = root.length_y;
  m_I.length_x = m_dims[0];
  m_I.length_y = m_dims[1];
  m_I.part_level = num_of_xforms;

  // Encoder and decoder might have different additional tasks.
  m_additional_initialization();
}

template class sperr::SPECK2D_INT<uint8_t>;
template class sperr::SPECK2D_INT<uint16_t>;
template class sperr::SPECK2D_INT<uint32_t>;
template class sperr::SPECK2D_INT<uint64_t>;
