#include "SPECK3D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_deposit_set(Set3D set)
{
  switch (set.num_elem()) {
    case 0:
      return;
    case 1: {
      auto id = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      m_morton_buf[set.morton_idx] = sperr::msb_position(m_coeff_buf[id]);
      return;
    }
    case 2: {
      // We directly deposit the 2 elements in `set` instead of performing another partition.
      //
      // Deposit the 1st element.
      auto id = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      auto morton_id = set.morton_idx;
      m_morton_buf[morton_id] = sperr::msb_position(m_coeff_buf[id]);

      // Deposit the 2nd element.
      if (set.length_x == 2)
        id++;
      else if (set.length_y == 2)
        id += m_dims[0];
      else
        id += m_dims[0] * m_dims[1];
      m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id]);

      return;
    }
    case 4: {
      const auto id = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      auto morton_id = set.morton_idx;

      if (set.length_x == 2 && set.length_y == 2) {
        // Element (0, 0, 0)
        m_morton_buf[morton_id] = sperr::msb_position(m_coeff_buf[id]);

        // Element (1, 0, 0)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id + 1]);

        // Element (0, 1, 0)
        auto id2 = id + m_dims[0];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (1, 1, 0)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[++id2]);

        return;
      }
      else if (set.length_x == 2 && set.length_z == 2) {
        // Element (0, 0, 0)
        m_morton_buf[morton_id] = sperr::msb_position(m_coeff_buf[id]);

        // Element (1, 0, 0)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id + 1]);

        // Element (0, 0, 1)
        auto id2 = id + m_dims[0] * m_dims[1];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (1, 0, 1)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[++id2]);

        return;
      }
      else if (set.length_y == 2 && set.length_z == 2) {
        // Element (0, 0, 0)
        m_morton_buf[morton_id] = sperr::msb_position(m_coeff_buf[id]);

        // Element (0, 1, 0)
        auto id2 = id + m_dims[0];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (0, 0, 1)
        id2 = id + m_dims[0] * m_dims[1];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (0, 1, 1)
        id2 += m_dims[0];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        return;
      }
      else
        break;  // Fall back to the recursive case.
    }
    case 8: {
      if (set.length_x == 2 && set.length_y == 2) {
        // Element (0, 0, 0)
        const auto id = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
        auto morton_id = set.morton_idx;
        m_morton_buf[morton_id] = sperr::msb_position(m_coeff_buf[id]);

        // Element (1, 0, 0)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id + 1]);

        // Element (0, 1, 0)
        auto id2 = id + m_dims[0];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (1, 1, 0)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[++id2]);

        // Element (0, 0, 1)
        id2 = id + m_dims[0] * m_dims[1];
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (1, 0, 1)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[++id2]);

        // Element (0, 1, 1)
        id2 = id + m_dims[0] * (m_dims[1] + 1);
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[id2]);

        // Element (1, 1, 1)
        m_morton_buf[++morton_id] = sperr::msb_position(m_coeff_buf[++id2]);

        return;
      }
      else
        break;  // Fall back to the recursive case.
    }
    default:
      break;  // Fall back to the recursive case.
  }

  // The recursive case.
  auto [subsets, lev] = m_partition_S_XYZ(set, 0);
  for (auto& sub : subsets)
    m_deposit_set(sub);
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_additional_initialization()
{
  // For the encoder, this function re-organizes the coefficients in a morton order.
  //
  m_morton_buf.resize(m_dims[0] * m_dims[1] * m_dims[2]);

  // The same traversing order as in `SPECK3D_INT::m_sorting_pass()`
  size_t morton_offset = 0;
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto& set = m_LIS[idx1][idx2];
      set.morton_idx = morton_offset;
      m_deposit_set(set);
      morton_offset += set.num_elem();
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool output)
{
  auto& set = m_LIS[idx1][idx2];
  auto is_sig = true;

  // If need to output, it means the current set has unknown significance.
  if (output) {
    auto first = m_morton_buf.begin() + set.morton_idx;
    is_sig = std::any_of(first, first + set.num_elem(),
                         [thld = m_morton_threshold](auto v) { return v >= thld; });
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.make_empty();  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_P(size_t idx, size_t morton, size_t& counter, bool output)
{
  bool is_sig = true;

  if (output) {
    assert(sperr::msb_position(m_coeff_buf[idx]) == m_morton_buf[morton]);
    is_sig = (m_morton_buf[morton] >= m_morton_threshold);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.wbit(m_sign_array.rbit(idx));
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_P_lite(size_t idx)
{
  auto is_sig = (m_coeff_buf[idx] >= m_threshold);
  m_bit_buffer.wbit(is_sig);

  if (is_sig) {
    m_bit_buffer.wbit(m_sign_array.rbit(idx));
    m_LSP_new.push_back(idx);
    m_LIP_mask.wfalse(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_bitplane_init()
{
  m_morton_threshold = sperr::msb_position(m_threshold);
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_refinement_extra()
{
  for (auto idx : m_LSP_new) {
    assert(m_coeff_buf[idx] >= m_threshold);
    m_coeff_buf[idx] -= m_threshold;
  }
}

template class sperr::SPECK3D_INT_ENC<uint64_t>;
template class sperr::SPECK3D_INT_ENC<uint32_t>;
template class sperr::SPECK3D_INT_ENC<uint16_t>;
template class sperr::SPECK3D_INT_ENC<uint8_t>;
