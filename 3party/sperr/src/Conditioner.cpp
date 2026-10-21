#include <algorithm>  // std::all_of()
#include <cassert>
#include <cmath>    // std::sqrt()
#include <cstring>  // std::memcpy()
#include <numeric>  // std::accumulate()
#include <type_traits>

#include "Conditioner.h"

auto sperr::Conditioner::condition(vecd_type& buf, dims_type dims) -> condi_type
{
  // The order of performing condition operations:
  // 1. Test constant. If it's a constant field, return immediately.
  // 2. Subtract mean;

  assert(!buf.empty());
  auto meta = std::array<bool, 8>{true,    // subtract mean
                                  false,   // unused
                                  false,   // unused
                                  false,   // unused
                                  false,   // unused
                                  false,   // unused
                                  false,   // unused
                                  false};  // [7]: is this a constant field?

  // Operation 1
  //
  if (std::all_of(buf.cbegin(), buf.cend(), [v0 = buf[0]](auto v) { return v == v0; })) {
    meta[m_constant_field_idx] = true;
    const double val = buf[0];
    const uint64_t nval = buf.size();
    //
    // Assemble a header of the following info and ordering:
    // meta   nval  val
    //
    auto header = condi_type();
    header[0] = sperr::pack_8_booleans(meta);
    size_t pos = 1;
    std::memcpy(header.data() + pos, &nval, sizeof(nval));
    pos += sizeof(nval);
    std::memcpy(header.data() + pos, &val, sizeof(val));

    return header;
  }

  // Operation 2
  //
  m_adjust_strides(buf.size());
  const auto mean = m_calc_mean(buf);
  std::for_each(buf.begin(), buf.end(), [mean](auto& v) { v -= mean; });

  // Assemble a header of the following info order:
  // meta   mean  (empty)
  //
  auto header = condi_type();
  header[0] = sperr::pack_8_booleans(meta);
  size_t pos = 1;
  std::memcpy(header.data() + pos, &mean, sizeof(mean));
  pos += sizeof(mean);
  while (pos < header.size())
    header[pos++] = 0;

  return header;
}

auto sperr::Conditioner::inverse_condition(vecd_type& buf,
                                           dims_type dims,
                                           condi_type header) -> RTNType
{
  // unpack meta bit fields
  auto meta = sperr::unpack_8_booleans(header[0]);
  size_t pos = 1;

  // Operation 1: if this is a constant field?
  //
  if (meta[m_constant_field_idx]) {
    uint64_t nval = 0;
    double val = 0.0;
    std::memcpy(&nval, header.data() + pos, sizeof(nval));
    pos += sizeof(nval);
    std::memcpy(&val, header.data() + pos, sizeof(val));

    buf.resize(nval);
    std::fill(buf.begin(), buf.end(), val);

    return RTNType::Good;
  }

  // Operation 2: add back the mean
  //
  double mean = 0.0;
  std::memcpy(&mean, header.data() + pos, sizeof(mean));
  std::for_each(buf.begin(), buf.end(), [mean](auto& v) { v += mean; });

  return RTNType::Good;
}

auto sperr::Conditioner::is_constant(uint8_t byte) const -> bool
{
  auto b8 = sperr::unpack_8_booleans(byte);
  return b8[m_constant_field_idx];
}

void sperr::Conditioner::save_q(condi_type& header, double q) const
{
  // Save at position 9, the same as in `retrieve_q()`.
  std::memcpy(header.data() + 9, &q, sizeof(q));
}

auto sperr::Conditioner::retrieve_q(condi_type header) const -> double
{
  assert(!is_constant(header[0]));
  double q = 0.0;
  // Retrieve at position 9, the same as in `save_q()`.
  std::memcpy(&q, header.data() + 9, sizeof(q));
  return q;
}

auto sperr::Conditioner::m_calc_mean(const vecd_type& buf) -> double
{
  assert(buf.size() % m_num_strides == 0);

  m_stride_buf.resize(m_num_strides);
  const size_t stride_size = buf.size() / m_num_strides;

  for (size_t s = 0; s < m_num_strides; s++) {
    auto begin = buf.begin() + stride_size * s;
    auto end = begin + stride_size;
    m_stride_buf[s] = std::accumulate(begin, end, double{0.0}) / static_cast<double>(stride_size);
  }

  double sum = std::accumulate(m_stride_buf.begin(), m_stride_buf.end(), double{0.0});

  return (sum / static_cast<double>(m_stride_buf.size()));
}

void sperr::Conditioner::m_adjust_strides(size_t len)
{
  m_num_strides = m_default_num_strides;
  if (len % m_num_strides == 0)
    return;

  size_t num = 0;

  // First, try to increase till 2^15 = 32,768
  for (num = m_num_strides; num <= 32'768; num++) {
    if (len % num == 0)
      break;
  }

  if (len % num == 0) {
    m_num_strides = num;
    return;
  }

  // Second, try to decrease till 1, at which point it must work.
  for (num = m_num_strides; num > 0; num--) {
    if (len % num == 0)
      break;
  }

  m_num_strides = num;
}
