#include "coding/arithmetic_codec.hpp"

#include "coding/writer.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

vector<uint32_t> FreqsToDistrTable(vector<uint32_t> const & origFreqs)
{
  uint64_t freqLowerBound = 0;
  while (1)
  {
    // Resulting distr table is initialized with first zero value.
    vector<uint64_t> result(1, 0);
    vector<uint32_t> freqs;
    uint64_t sum = 0;
    uint64_t minFreq = ~uint64_t(0);
    for (uint32_t i = 0; i < origFreqs.size(); ++i)
    {
      uint32_t freq = origFreqs[i];
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
    for (uint32_t i = 1; i < result.size(); ++i)
    {
      result[i] = (result[i] << DISTR_SHIFT) / sum;
      if (freqs[i - 1] > 0 && (result[i] - result[i - 1] == 0))
      {
        hasDegradedZeroInterval = true;
        break;
      }
    }
    if (!hasDegradedZeroInterval) return vector<uint32_t>(result.begin(), result.end());
    ++freqLowerBound;
  }  
}

ArithmeticEncoder::ArithmeticEncoder(vector<uint32_t> const & distrTable)
  : m_begin(0), m_size(-1), m_distrTable(distrTable) {}

void ArithmeticEncoder::Encode(uint32_t symbol)
{
  CHECK_LESS(symbol + 1, m_distrTable.size(), ());
  uint32_t distrBegin = m_distrTable[symbol];
  uint32_t distrEnd = m_distrTable[symbol + 1];
  CHECK_LESS(distrBegin, distrEnd, ());
  uint32_t prevBegin = m_begin;
  m_begin += (m_size >> DISTR_SHIFT) * distrBegin;
  m_size = (m_size >> DISTR_SHIFT) * (distrEnd - distrBegin);
  if (m_begin < prevBegin) PropagateCarry();
  while (m_size < (uint32_t(1) << 24))
  {
    m_output.push_back(uint8_t(m_begin >> 24));
    m_begin <<= 8;
    m_size <<= 8;
  }
}

vector<uint8_t> ArithmeticEncoder::Finalize()
{
  CHECK_GREATER(m_size, 0, ());
  uint32_t last = m_begin + m_size - 1;
  if (last < m_begin)
  {
    PropagateCarry();
  }
  else
  {
    uint32_t resultHiBits = bits::NumHiZeroBits32(m_begin ^ last) + 1;
    uint32_t value = last & (~uint32_t(0) << (32 - resultHiBits));
    while (value != 0)
    {
      m_output.push_back(uint8_t(value >> 24));
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

ArithmeticDecoder::ArithmeticDecoder(Reader & reader, vector<uint32_t> const & distrTable)
  : m_codeValue(0), m_size(-1), m_reader(reader), m_serialCur(0),
    m_serialEnd(reader.Size()), m_distrTable(distrTable)
{
  for (uint32_t i = 0; i < sizeof(m_codeValue); ++i)
  {
    m_codeValue <<= 8;
    m_codeValue |= ReadCodeByte();
  }
}

uint32_t ArithmeticDecoder::Decode()
{
  uint32_t l = 0, r = m_distrTable.size(), m = 0;
  uint32_t shiftedSize = m_size >> DISTR_SHIFT;
  while (r - l > 1)
  {
    m = (l + r) / 2;
    uint32_t intervalBegin = shiftedSize * m_distrTable[m];
    if (intervalBegin <= m_codeValue) l = m; else r = m;
  }
  uint32_t symbol = l;
  m_codeValue -= shiftedSize * m_distrTable[symbol];
  m_size = shiftedSize * (m_distrTable[symbol + 1] - m_distrTable[symbol]);
  while (m_size < (uint32_t(1) << 24))
  {
    m_codeValue <<= 8;
    m_size <<= 8;
    m_codeValue |= ReadCodeByte();
  }
  return symbol;
}

uint8_t ArithmeticDecoder::ReadCodeByte()
{
  if (m_serialCur >= m_serialEnd) return 0;
  else
  {
    uint8_t result = 0;
    m_reader.Read(m_serialCur, &result, 1);
    ++m_serialCur;
    return result;
  }
}
