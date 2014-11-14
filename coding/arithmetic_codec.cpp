#include "arithmetic_codec.hpp"

#include "writer.hpp"
#include "reader.hpp"

#include "../base/assert.hpp"

using std::vector;

namespace {
  inline uint32_t NumHiZeroBits32(uint32_t n) {
    uint32_t result = 0;
    while ((n & (uint32_t(1) << 31)) == 0) { ++result; n <<= 1; }
    return result;
  }
}

vector<uint32_t> FreqsToDistrTable(vector<uint32_t> const & orig_freqs) {
  uint64_t freq_lower_bound = 0;
  while (1) {
    // Resulting distr table is initialized with first zero value.
    vector<uint32_t> result(1, 0);
    vector<uint32_t> freqs;
    uint32_t sum = 0;
    uint64_t min_freq = ~uint64_t(0);
    for (uint32_t i = 0; i < orig_freqs.size(); ++i) {
      uint32_t freq = orig_freqs[i];
      if (freq > 0 && freq < min_freq) min_freq = freq;
      if (freq > 0 && freq < freq_lower_bound) freq = freq_lower_bound;
      freqs.push_back(freq);
      sum += freq;
      result.push_back(sum);
    }
    if (freq_lower_bound == 0) freq_lower_bound = min_freq;
    // This flag shows that some interval with non-zero freq has
    // degraded to zero interval in normalized distribution table.
    bool has_degraded_zero_interval = false;
    for (uint32_t i = 1; i < result.size(); ++i) {
      result[i] = (uint64_t(result[i]) << c_distr_shift) / uint64_t(sum);
      if (freqs[i - 1] > 0 && (result[i] - result[i - 1] == 0)) {
        has_degraded_zero_interval = true;
        break;
      }
    }
    if (!has_degraded_zero_interval) return result;
    ++freq_lower_bound;
  }
}

ArithmeticEncoder::ArithmeticEncoder(vector<uint32_t> const & distr_table)
  : begin_(0), size_(-1), distr_table_(distr_table) {}

void ArithmeticEncoder::Encode(uint32_t symbol) {
  ASSERT_LESS(symbol + 1, distr_table_.size(), ());
  uint32_t distr_begin = distr_table_[symbol];
  uint32_t distr_end = distr_table_[symbol + 1];
  ASSERT_LESS(distr_begin, distr_end, ());
  uint32_t prev_begin = begin_;
  begin_ += (size_ >> c_distr_shift) * distr_begin;
  size_ = (size_ >> c_distr_shift) * (distr_end - distr_begin);
  if (begin_ < prev_begin) PropagateCarry();
  while (size_ < (uint32_t(1) << 24)) {
    output_.push_back(uint8_t(begin_ >> 24));
    begin_ <<= 8;
    size_ <<= 8;
  }
}

vector<uint8_t> ArithmeticEncoder::Finalize() {
  ASSERT_GREATER(size_, 0, ());
  uint32_t last = begin_ + size_ - 1;
  if (last < begin_) {
    PropagateCarry();
  } else {
    uint32_t result_hi_bits = NumHiZeroBits32(begin_ ^ last) + 1;
    uint32_t value = last & (~uint32_t(0) << (32 - result_hi_bits));
    while (value != 0) {
      output_.push_back(uint8_t(value >> 24));
      value <<= 8;
    }
  }
  begin_ = 0;
  size_ = 0;
  return output_;
}

void ArithmeticEncoder::PropagateCarry() {
  int i = output_.size() - 1;
  while (i >= 0 && output_[i] == 0xFF) {
    output_[i] = 0;
    --i;
  }
  ASSERT_GREATER_OR_EQUAL(i, 0, ());
  ++output_[i];
}

ArithmeticDecoder::ArithmeticDecoder(Reader & reader, vector<uint32_t> const & distr_table)
  : code_value_(0), size_(-1), reader_(reader), serial_cur_(0), serial_end_(reader.Size()), distr_table_(distr_table) {
  for (uint32_t i = 0; i < sizeof(code_value_); ++i) {
    code_value_ <<= 8;
    code_value_ |= ReadCodeByte();
  }
}

uint32_t ArithmeticDecoder::Decode() {
  uint32_t l = 0, r = distr_table_.size(), m = 0;
  while (r - l > 1) {
    m = (l + r) / 2;
    uint32_t interval_begin = (size_ >> c_distr_shift) * distr_table_[m];
    if (interval_begin <= code_value_) l = m; else r = m;
  }
  uint32_t symbol = l;
  code_value_ -= (size_ >> c_distr_shift) * distr_table_[symbol];
  size_ = (size_ >> c_distr_shift) * (distr_table_[symbol + 1] - distr_table_[symbol]);
  while (size_ < (uint32_t(1) << 24)) {
    code_value_ <<= 8;
    size_ <<= 8;
    code_value_ |= ReadCodeByte();
  }
  return symbol;
}

uint8_t ArithmeticDecoder::ReadCodeByte() {
  if (serial_cur_ >= serial_end_) return 0;
  else {
    uint8_t result = 0;
    reader_.Read(serial_cur_, &result, 1);
    ++serial_cur_;
    return result;
  }
}
