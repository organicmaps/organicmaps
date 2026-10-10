#include "SPECK3D_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

#if __cplusplus >= 202002L
#include <bit>
#endif

template <typename T>
void sperr::SPECK3D_INT<T>::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it =
        std::remove_if(list.begin(), list.end(), [](const auto& s) { return s.num_elem() == 0; });
    list.erase(it, list.end());
  }
}

template <typename T>
void sperr::SPECK3D_INT<T>::m_initialize_lists()
{
  std::array<size_t, 3> num_of_parts;  // how many times each dimension could be partitioned?
  num_of_parts[0] = sperr::num_of_partitions(m_dims[0]);
  num_of_parts[1] = sperr::num_of_partitions(m_dims[1]);
  num_of_parts[2] = sperr::num_of_partitions(m_dims[2]);
  size_t num_of_sizes = std::accumulate(num_of_parts.cbegin(), num_of_parts.cend(), 1ul);

  // Initialize LIS
  if (m_LIS.size() < num_of_sizes)
    m_LIS.resize(num_of_sizes);
  std::for_each(m_LIS.begin(), m_LIS.end(), [](auto& list) { list.clear(); });

  // Starting from a set representing the whole volume, identify the smaller
  //    subsets and put them in the LIS accordingly.
  //    Note that it truncates 64-bit ints to 16-bit ints here, but should be OK.
  Set3D big;
  big.length_x = static_cast<uint16_t>(m_dims[0]);
  big.length_y = static_cast<uint16_t>(m_dims[1]);
  big.length_z = static_cast<uint16_t>(m_dims[2]);

  auto curr_lev = uint16_t{0};

  const auto dyadic = sperr::can_use_dyadic(m_dims);
  if (dyadic) {
    for (size_t i = 0; i < *dyadic; i++) {
      auto [subsets, next_lev] = m_partition_S_XYZ(big, curr_lev);
      big = subsets[0];
      for (auto it = std::next(subsets.cbegin()); it != subsets.cend(); ++it)
        m_LIS[next_lev].emplace_back(*it);
      curr_lev = next_lev;
    }
  }
  else {
    const auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
    const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);
    size_t xf = 0;
    while (xf < num_xforms_xy && xf < num_xforms_z) {
      auto [subsets, next_lev] = m_partition_S_XYZ(big, curr_lev);
      big = subsets[0];
      for (auto it = std::next(subsets.cbegin()); it != subsets.cend(); ++it)
        m_LIS[next_lev].emplace_back(*it);
      curr_lev = next_lev;
      xf++;
    }

    // One of these two conditions will happen.
    if (xf < num_xforms_xy) {
      while (xf < num_xforms_xy) {
        auto [subsets, next_lev] = m_partition_S_XY(big, curr_lev);
        big = subsets[0];
        for (auto it = std::next(subsets.cbegin()); it != subsets.cend(); ++it)
          m_LIS[next_lev].emplace_back(*it);
        curr_lev = next_lev;
        xf++;
      }
    }
    else if (xf < num_xforms_z) {
      while (xf < num_xforms_z) {
        auto [subsets, next_lev] = m_partition_S_Z(big, curr_lev);
        big = subsets[0];
        m_LIS[next_lev].emplace_back(subsets[1]);
        curr_lev = next_lev;
        xf++;
      }
    }
  }

  // Right now big is the set that's most likely to be significant, so insert
  // it at the front of it's corresponding vector. One-time expense.
  m_LIS[curr_lev].insert(m_LIS[curr_lev].begin(), big);

  // Encoder and decoder might have different additional tasks.
  m_additional_initialization();
}

template <typename T>
void sperr::SPECK3D_INT<T>::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first!
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    auto value = m_LIP_mask.rlong(i);

#if __cplusplus >= 202002L
    while (value) {
      auto j = std::countr_zero(value);
      m_process_P_lite(i + j);
      value &= value - 1;
    }
#else
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1})
          m_process_P_lite(i + j);
      }
    }
#endif
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.rbit(i))
      m_process_P_lite(i);
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto set = m_LIS[idx1][idx2];

  if (set.length_x == 2 && set.length_y == 2 && set.length_z == 2) {  // tail ellison case
    size_t sig_counter = 0;
    bool need_decide = true;

    // Element (0, 0, 0)
    const auto id = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
    auto mort = set.morton_idx;
    m_LIP_mask.wtrue(id);
    m_process_P(id, mort, sig_counter, need_decide);

    // Element (1, 0, 0)
    auto id2 = id + 1;
    m_LIP_mask.wtrue(id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (0, 1, 0)
    id2 = id + m_dims[0];
    m_LIP_mask.wtrue(id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (1, 1, 0)
    m_LIP_mask.wtrue(++id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (0, 0, 1)
    id2 = id + m_dims[0] * m_dims[1];
    m_LIP_mask.wtrue(id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (1, 0, 1)
    m_LIP_mask.wtrue(++id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (0, 1, 1)
    id2 = id + m_dims[0] * (m_dims[1] + 1);
    m_LIP_mask.wtrue(id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);

    // Element (1, 1, 1)
    need_decide = sig_counter != 0;
    m_LIP_mask.wtrue(++id2);
    m_process_P(id2, ++mort, sig_counter, need_decide);
  }
  else {  // normal recursion case
          // Get its 8 subsets, and move the empty ones to the end.
    auto [subsets, next_lev] = m_partition_S_XYZ(set, uint16_t(idx1));
    const auto set_end =
        std::remove_if(subsets.begin(), subsets.end(), [](auto& s) { return s.num_elem() == 0; });

    // Counter for the number of discovered significant sets.
    //    If no significant subset is found yet, and we're already looking at the last subset,
    //    then we know that this last subset IS significant.
    size_t sig_counter = 0;
    for (auto it = subsets.begin(); it != set_end; ++it) {
      bool need_decide = (sig_counter != 0 || it + 1 != set_end);
      if (it->num_elem() == 1) {
        auto idx = it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x;
        m_LIP_mask.wtrue(idx);
        m_process_P(idx, it->morton_idx, sig_counter, need_decide);
      }
      else {
        m_LIS[next_lev].emplace_back(*it);
        const auto newidx2 = m_LIS[next_lev].size() - 1;
        m_process_S(next_lev, newidx2, sig_counter, need_decide);
      }
    }
  }
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_XYZ(Set3D set, uint16_t lev) const
    -> std::tuple<std::array<Set3D, 8>, uint16_t>
{
  // Integer promotion rules (https://en.cppreference.com/w/c/language/conversion) say that types
  //    shorter than `int` are implicitly promoted to be `int` to perform calculations, so just
  //    keep them as `int` here because they'll involve in calculations later.
  //
  const auto split_x = std::array<int, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<int, 2>{set.length_y - set.length_y / 2, set.length_y / 2};
  const auto split_z = std::array<int, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  const auto tmp = std::array<uint8_t, 2>{0, 1};
  lev += tmp[split_x[1] != 0];
  lev += tmp[split_y[1] != 0];
  lev += tmp[split_z[1] != 0];

  auto subsets = std::tuple<std::array<Set3D, 8>, uint16_t>();
  std::get<1>(subsets) = lev;
  auto morton_offset = set.morton_idx;

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  auto& sub0 = std::get<0>(subsets)[0];
  sub0.morton_idx = morton_offset;
  sub0.start_x = set.start_x;
  sub0.start_y = set.start_y;
  sub0.start_z = set.start_z;
  sub0.length_x = split_x[0];
  sub0.length_y = split_y[0];
  sub0.length_z = split_z[0];

  // subset (1, 0, 0)
  auto& sub1 = std::get<0>(subsets)[1];
  morton_offset += sub0.num_elem();
  sub1.morton_idx = morton_offset;
  sub1.start_x = set.start_x + split_x[0];
  sub1.start_y = set.start_y;
  sub1.start_z = set.start_z;
  sub1.length_x = split_x[1];
  sub1.length_y = split_y[0];
  sub1.length_z = split_z[0];

  // subset (0, 1, 0)
  auto& sub2 = std::get<0>(subsets)[2];
  morton_offset += sub1.num_elem();
  sub2.morton_idx = morton_offset;
  sub2.start_x = set.start_x;
  sub2.start_y = set.start_y + split_y[0];
  sub2.start_z = set.start_z;
  sub2.length_x = split_x[0];
  sub2.length_y = split_y[1];
  sub2.length_z = split_z[0];

  // subset (1, 1, 0)
  auto& sub3 = std::get<0>(subsets)[3];
  morton_offset += sub2.num_elem();
  sub3.morton_idx = morton_offset;
  sub3.start_x = set.start_x + split_x[0];
  sub3.start_y = set.start_y + split_y[0];
  sub3.start_z = set.start_z;
  sub3.length_x = split_x[1];
  sub3.length_y = split_y[1];
  sub3.length_z = split_z[0];

  // subset (0, 0, 1)
  auto& sub4 = std::get<0>(subsets)[4];
  morton_offset += sub3.num_elem();
  sub4.morton_idx = morton_offset;
  sub4.start_x = set.start_x;
  sub4.start_y = set.start_y;
  sub4.start_z = set.start_z + split_z[0];
  sub4.length_x = split_x[0];
  sub4.length_y = split_y[0];
  sub4.length_z = split_z[1];

  // subset (1, 0, 1)
  auto& sub5 = std::get<0>(subsets)[5];
  morton_offset += sub4.num_elem();
  sub5.morton_idx = morton_offset;
  sub5.start_x = set.start_x + split_x[0];
  sub5.start_y = set.start_y;
  sub5.start_z = set.start_z + split_z[0];
  sub5.length_x = split_x[1];
  sub5.length_y = split_y[0];
  sub5.length_z = split_z[1];

  // subset (0, 1, 1)
  auto& sub6 = std::get<0>(subsets)[6];
  morton_offset += sub5.num_elem();
  sub6.morton_idx = morton_offset;
  sub6.start_x = set.start_x;
  sub6.start_y = set.start_y + split_y[0];
  sub6.start_z = set.start_z + split_z[0];
  sub6.length_x = split_x[0];
  sub6.length_y = split_y[1];
  sub6.length_z = split_z[1];

  // subset (1, 1, 1)
  auto& sub7 = std::get<0>(subsets)[7];
  morton_offset += sub6.num_elem();
  sub7.morton_idx = morton_offset;
  sub7.start_x = set.start_x + split_x[0];
  sub7.start_y = set.start_y + split_y[0];
  sub7.start_z = set.start_z + split_z[0];
  sub7.length_x = split_x[1];
  sub7.length_y = split_y[1];
  sub7.length_z = split_z[1];

  return subsets;
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_XY(Set3D set, uint16_t lev) const
    -> std::tuple<std::array<Set3D, 4>, uint16_t>
{
  // This partition scheme is only used during initialization; no need to calculate morton offset.
  // The correct morton offset will be calculated during initialization.

  const auto split_x = std::array<int, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<int, 2>{set.length_y - set.length_y / 2, set.length_y / 2};

  const auto tmp = std::array<uint8_t, 2>{0, 1};
  lev += tmp[split_x[1] != 0];
  lev += tmp[split_y[1] != 0];

  auto subsets = std::tuple<std::array<Set3D, 4>, uint16_t>();
  std::get<1>(subsets) = lev;
  const auto offsets = std::array<size_t, 3>{1, 2, 4};

  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  size_t sub_i = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = std::get<0>(subsets)[sub_i];
  sub0.start_x = set.start_x;
  sub0.start_y = set.start_y;
  sub0.start_z = set.start_z;
  sub0.length_x = split_x[0];
  sub0.length_y = split_y[0];
  sub0.length_z = set.length_z;

  // subset (1, 0, 0)
  sub_i = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = std::get<0>(subsets)[sub_i];
  sub1.start_x = set.start_x + split_x[0];
  sub1.start_y = set.start_y;
  sub1.start_z = set.start_z;
  sub1.length_x = split_x[1];
  sub1.length_y = split_y[0];
  sub1.length_z = set.length_z;

  // subset (0, 1, 0)
  sub_i = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = std::get<0>(subsets)[sub_i];
  sub2.start_x = set.start_x;
  sub2.start_y = set.start_y + split_y[0];
  sub2.start_z = set.start_z;
  sub2.length_x = split_x[0];
  sub2.length_y = split_y[1];
  sub2.length_z = set.length_z;

  // subset (1, 1, 0)
  sub_i = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = std::get<0>(subsets)[sub_i];
  sub3.start_x = set.start_x + split_x[0];
  sub3.start_y = set.start_y + split_y[0];
  sub3.start_z = set.start_z;
  sub3.length_x = split_x[1];
  sub3.length_y = split_y[1];
  sub3.length_z = set.length_z;

  return subsets;
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_Z(Set3D set, uint16_t lev) const
    -> std::tuple<std::array<Set3D, 2>, uint16_t>
{
  // This partition scheme is only used during initialization; no need to calculate morton offset.
  // The correct morton offset will be calculated during initialization.

  const auto split_z = std::array<int, 2>{set.length_z - set.length_z / 2, set.length_z / 2};
  if (split_z[1] != 0)
    lev++;

  auto subsets = std::tuple<std::array<Set3D, 2>, uint16_t>();
  std::get<1>(subsets) = lev;

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  auto& sub0 = std::get<0>(subsets)[0];
  sub0.start_x = set.start_x;
  sub0.length_x = set.length_x;
  sub0.start_y = set.start_y;
  sub0.length_y = set.length_y;
  sub0.start_z = set.start_z;
  sub0.length_z = split_z[0];

  // subset (0, 0, 1)
  auto& sub1 = std::get<0>(subsets)[1];
  sub1.start_x = set.start_x;
  sub1.length_x = set.length_x;
  sub1.start_y = set.start_y;
  sub1.length_y = set.length_y;
  sub1.start_z = set.start_z + split_z[0];
  sub1.length_z = split_z[1];

  return subsets;
}

template class sperr::SPECK3D_INT<uint64_t>;
template class sperr::SPECK3D_INT<uint32_t>;
template class sperr::SPECK3D_INT<uint16_t>;
template class sperr::SPECK3D_INT<uint8_t>;
