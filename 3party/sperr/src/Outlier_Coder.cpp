#include "Outlier_Coder.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cfloat>  // FLT_ROUNDS
#include <cmath>

sperr::Outlier::Outlier(size_t p, double e) : pos(p), err(e) {}

void sperr::Outlier_Coder::add_outlier(Outlier out)
{
  m_LOS.push_back(out);
}

void sperr::Outlier_Coder::use_outlier_list(std::vector<Outlier> los)
{
  m_LOS = std::move(los);
}

void sperr::Outlier_Coder::set_length(size_t len)
{
  m_total_len = len;
}

void sperr::Outlier_Coder::set_tolerance(double tol)
{
  m_tol = tol;
}

auto sperr::Outlier_Coder::view_outlier_list() const -> const std::vector<Outlier>&
{
  return m_LOS;
}

void sperr::Outlier_Coder::append_encoded_bitstream(vec8_type& buf) const
{
  // Just append the bitstream produced by `m_encoder` is fine.
  std::visit([&buf](auto&& enc) { enc.append_encoded_bitstream(buf); }, m_encoder);
}

auto sperr::Outlier_Coder::get_stream_full_len(const void* p) const -> size_t
{
  return std::visit([p](auto&& dec) { return dec.get_stream_full_len(p); }, m_decoder);
}

auto sperr::Outlier_Coder::use_bitstream(const void* p, size_t len) -> RTNType
{
  // Decide on the integer length to use.
  const auto num_bitplanes = speck_int_get_num_bitplanes(p);
  if (num_bitplanes <= 8)
    m_instantiate_uvec_coders(UINTType::UINT8);
  else if (num_bitplanes <= 16)
    m_instantiate_uvec_coders(UINTType::UINT16);
  else if (num_bitplanes <= 32)
    m_instantiate_uvec_coders(UINTType::UINT32);
  else
    m_instantiate_uvec_coders(UINTType::UINT64);

  // Clean up data structures.
  m_sign_array.resize(0);
  m_LOS.clear();
  std::visit([](auto&& vec) { vec.clear(); }, m_vals_ui);

  // Ask the decoder to use the bitstream directly.
  std::visit([p, len](auto&& dec) { dec.use_bitstream(p, len); }, m_decoder);

  return RTNType::Good;
}

auto sperr::Outlier_Coder::encode() -> RTNType
{
  // Sanity check: whether we can proceed with encoding.
  if (m_total_len == 0 || m_tol <= 0.0 || m_LOS.empty())
    return RTNType::Error;
  if (std::any_of(m_LOS.cbegin(), m_LOS.cend(), [len = m_total_len, tol = m_tol](auto out) {
        return out.pos >= len || std::abs(out.err) <= tol;
      }))
    return RTNType::Error;

  // Step 1: find the biggest magnitude of outlier errors, and then instantiate data structures.
  auto maxerr = *std::max_element(m_LOS.cbegin(), m_LOS.cend(), [](auto v1, auto v2) {
    return std::abs(v1.err) < std::abs(v2.err);
  });
  std::fesetround(FE_TONEAREST);
  assert(FE_TONEAREST == std::fegetround());
  assert(FLT_ROUNDS == 1);
  std::feclearexcept(FE_INVALID);
  auto maxint = std::llrint(std::abs(maxerr.err));
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;

  if (maxint <= std::numeric_limits<uint8_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT8);
  else if (maxint <= std::numeric_limits<uint16_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT16);
  else if (maxint <= std::numeric_limits<uint32_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT32);
  else
    m_instantiate_uvec_coders(UINTType::UINT64);

  // Step 2: quantize the outliers.
  m_quantize();

  // Step 3: integer SPECK encoding.
  std::visit([len = m_total_len](auto&& enc) { enc.set_dims({len, 1, 1}); }, m_encoder);
  auto rtn = RTNType::Good;
  switch (m_encoder.index()) {
    case 0:
      rtn = std::get<0>(m_encoder).use_coeffs(std::move(std::get<0>(m_vals_ui)),
                                              std::move(m_sign_array));
      break;
    case 1:
      rtn = std::get<1>(m_encoder).use_coeffs(std::move(std::get<1>(m_vals_ui)),
                                              std::move(m_sign_array));
      break;
    case 2:
      rtn = std::get<2>(m_encoder).use_coeffs(std::move(std::get<2>(m_vals_ui)),
                                              std::move(m_sign_array));
      break;
    default:
      rtn = std::get<3>(m_encoder).use_coeffs(std::move(std::get<3>(m_vals_ui)),
                                              std::move(m_sign_array));
  }
  if (rtn != RTNType::Good)
    return rtn;

  std::visit([](auto&& enc) { enc.encode(); }, m_encoder);

  return RTNType::Good;
}

auto sperr::Outlier_Coder::decode() -> RTNType
{
  // Sanity check
  if (m_total_len == 0 || m_tol <= 0.0)
    return RTNType::Error;

  // Step 1: Integer SPECK decode
  std::visit([len = m_total_len](auto&& dec) { dec.set_dims({len, 1, 1}); }, m_decoder);
  std::visit([](auto&& dec) { dec.decode(); }, m_decoder);
  std::visit([&vec = m_vals_ui](auto&& dec) { vec = dec.release_coeffs(); }, m_decoder);
  m_sign_array = std::visit([](auto&& dec) { return dec.release_signs(); }, m_decoder);

  // Step 2: Inverse quantization
  m_inverse_quantize();

  return RTNType::Good;
}

void sperr::Outlier_Coder::m_instantiate_uvec_coders(UINTType type)
{
  switch (type) {
    case UINTType::UINT8:
      if (m_vals_ui.index() != 0)
        m_vals_ui.emplace<0>();
      if (m_encoder.index() != 0)
        m_encoder.emplace<0>();
      if (m_decoder.index() != 0)
        m_decoder.emplace<0>();
      break;
    case UINTType::UINT16:
      if (m_vals_ui.index() != 1)
        m_vals_ui.emplace<1>();
      if (m_encoder.index() != 1)
        m_encoder.emplace<1>();
      if (m_decoder.index() != 1)
        m_decoder.emplace<1>();
      break;
    case UINTType::UINT32:
      if (m_vals_ui.index() != 2)
        m_vals_ui.emplace<2>();
      if (m_encoder.index() != 2)
        m_encoder.emplace<2>();
      if (m_decoder.index() != 2)
        m_decoder.emplace<2>();
      break;
    default:
      if (m_vals_ui.index() != 3)
        m_vals_ui.emplace<3>();
      if (m_encoder.index() != 3)
        m_encoder.emplace<3>();
      if (m_decoder.index() != 3)
        m_decoder.emplace<3>();
  }
}

void sperr::Outlier_Coder::m_quantize()
{
  std::visit([len = m_total_len](auto&& vec) { vec.assign(len, 0); }, m_vals_ui);
  m_sign_array.resize(m_total_len);
  m_sign_array.reset_true();

  std::visit(
      [&los = m_LOS, &signs = m_sign_array, tol = m_tol](auto&& vec) {
        auto inv = 1.0 / tol;
        for (auto out : los) {
          auto ll = std::llrint(out.err * inv);
          signs.wbit(out.pos, ll >= 0);
          vec[out.pos] = std::abs(ll);
        }
      },
      m_vals_ui);
}

void sperr::Outlier_Coder::m_inverse_quantize()
{
  m_LOS.clear();

  // First, bring all non-zero integer correctors to `m_LOS`.
  std::visit(
      [&los = m_LOS](auto&& vec) {
        for (size_t i = 0; i < vec.size(); i++)
          switch (vec[i]) {
            case 0:
              break;
            case 1:
              los.emplace_back(i, 1.1);
              break;
            default:
              los.emplace_back(i, static_cast<double>(vec[i]) - 0.25);
          }
      },
      m_vals_ui);

  // Second, restore the floating-point correctors.
  const auto tmp = std::array<double, 2>{-1.0, 1.0};
  std::transform(m_LOS.cbegin(), m_LOS.cend(), m_LOS.begin(),
                 [q = m_tol, &signs = m_sign_array, tmp](auto los) {
                   auto b = signs.rbit(los.pos);
                   los.err *= (q * tmp[b]);
                   return los;
                 });
}
