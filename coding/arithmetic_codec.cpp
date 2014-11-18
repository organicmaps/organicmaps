#include "arithmetic_codec.hpp"

#include "writer.hpp"
#include "reader.hpp"

#include "../base/assert.hpp"

namespace {
  inline u32 NumHiZeroBits32(u32 n)
  {
    u32 result = 0;
    while ((n & (u32(1) << 31)) == 0) { ++result; n <<= 1; }
    return result;
  }
}

vector<u32> FreqsToDistrTable(vector<u32> const & origFreqs)
{
  u64 freqLowerBound = 0;
  while (1)
  {
    // Resulting distr table is initialized with first zero value.
    vector<u64> result(1, 0);
    vector<u32> freqs;
    u64 sum = 0;
    u64 minFreq = ~u64(0);
    for (u32 i = 0; i < origFreqs.size(); ++i)
    {
      u32 freq = origFreqs[i];
      if (freq > 0 && freq < minFreq) minFreq = freq;
      if (freq > 0 && freq < freqLowerBound) freq = freqLowerBound;
      freqs.push_back(freq);
      sum += freq;
      result.push_back(sum);
    }
    if (freqLowerBound == 0) freqLowerBound = minFreq;
    // This flag shows that some interval with non-zero freq has
    // degraded to zero interval in normalized distribution table.
    bool hasDegradedZeroInterval = false;
    for (u32 i = 1; i < result.size(); ++i)
    {
      result[i] = (result[i] << DISTR_SHIFT) / sum;
      if (freqs[i - 1] > 0 && (result[i] - result[i - 1] == 0))
      {
        hasDegradedZeroInterval = true;
        break;
      }
    }
    if (!hasDegradedZeroInterval) {
      // Convert distr_table to 32-bit vector, although currently only 17 bits are used.
      vector<u32> distr_table;
      for (u32 i = 0; i < result.size(); ++i) distr_table.push_back(result[i]);
      return distr_table;
    }
    ++freqLowerBound;
  }  
}

ArithmeticEncoder::ArithmeticEncoder(vector<u32> const & distrTable)
  : m_begin(0), m_size(-1), m_distrTable(distrTable) {}

void ArithmeticEncoder::Encode(u32 symbol)
{
  CHECK_LESS(symbol + 1, m_distrTable.size(), ());
  u32 distrBegin = m_distrTable[symbol];
  u32 distrEnd = m_distrTable[symbol + 1];
  CHECK_LESS(distrBegin, distrEnd, ());
  u32 prevBegin = m_begin;
  m_begin += (m_size >> DISTR_SHIFT) * distrBegin;
  m_size = (m_size >> DISTR_SHIFT) * (distrEnd - distrBegin);
  if (m_begin < prevBegin) PropagateCarry();
  while (m_size < (u32(1) << 24))
  {
    m_output.push_back(u8(m_begin >> 24));
    m_begin <<= 8;
    m_size <<= 8;
  }
}

vector<u8> ArithmeticEncoder::Finalize()
{
  CHECK_GREATER(m_size, 0, ());
  u32 last = m_begin + m_size - 1;
  if (last < m_begin)
  {
    PropagateCarry();
  }
  else
  {
    u32 resultHiBits = NumHiZeroBits32(m_begin ^ last) + 1;
    u32 value = last & (~u32(0) << (32 - resultHiBits));
    while (value != 0)
    {
      m_output.push_back(u8(value >> 24));
      value <<= 8;
    }
  }
  m_begin = 0;
  m_size = 0;
  return m_output;
}

void ArithmeticEncoder::PropagateCarry()
{
  int i = m_output.size() - 1;
  while (i >= 0 && m_output[i] == 0xFF)
  {
    m_output[i] = 0;
    --i;
  }
  CHECK_GREATER_OR_EQUAL(i, 0, ());
  ++m_output[i];
}

ArithmeticDecoder::ArithmeticDecoder(Reader & reader, vector<u32> const & distrTable)
  : m_codeValue(0), m_size(-1), m_reader(reader), m_serialCur(0),
    m_serialEnd(reader.Size()), m_distrTable(distrTable)
{
  for (u32 i = 0; i < sizeof(m_codeValue); ++i)
  {
    m_codeValue <<= 8;
    m_codeValue |= ReadCodeByte();
  }
}

u32 ArithmeticDecoder::Decode()
{
  u32 l = 0, r = m_distrTable.size(), m = 0;
  u32 shiftedSize = m_size >> DISTR_SHIFT;
  while (r - l > 1)
  {
    m = (l + r) / 2;
    u32 intervalBegin = shiftedSize * m_distrTable[m];
    if (intervalBegin <= m_codeValue) l = m; else r = m;
  }
  u32 symbol = l;
  m_codeValue -= shiftedSize * m_distrTable[symbol];
  m_size = shiftedSize * (m_distrTable[symbol + 1] - m_distrTable[symbol]);
  while (m_size < (u32(1) << 24))
  {
    m_codeValue <<= 8;
    m_size <<= 8;
    m_codeValue |= ReadCodeByte();
  }
  return symbol;
}

u8 ArithmeticDecoder::ReadCodeByte()
{
  if (m_serialCur >= m_serialEnd) return 0;
  else
  {
    u8 result = 0;
    m_reader.Read(m_serialCur, &result, 1);
    ++m_serialCur;
    return result;
  }
}
