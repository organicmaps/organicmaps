#pragma once
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/huffman.hpp"
#include "coding/bit_streams.hpp"
#include "coding/trie.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/vector.hpp"

#include "3party/succinct/rs_bit_vector.hpp"

namespace trie
{
template <class TReader, class TValueReader, class TEdgeValueReader>
class TopologyAndOffsets
{
public:
  using ValueType = typename TValueReader::ValueType;
  using EdgeValueType = typename TEdgeValueReader::ValueType;

  TopologyAndOffsets(TReader const & reader, TValueReader const & valueReader,
                     TEdgeValueReader const & edgeValueReader)
    : m_reader(reader), m_valueReader(valueReader), m_edgeValueReader(edgeValueReader)
  {
    Parse();
  }

  succinct::rs_bit_vector const & GetTopology() const { return m_trieTopology; }
  uint32_t NumNodes() const { return m_numNodes; }
  coding::HuffmanCoder const & GetEncoding() const { return m_huffman; }
  bool NodeIsFinal(uint32_t id) const { return m_finalNodeIndex[id] >= 0; }
  uint32_t Offset(uint32_t id) const { return m_offsetTable[m_finalNodeIndex[id]]; }
  uint32_t ValueListSize(uint32_t id)
  {
    int finalId = m_finalNodeIndex[id];
    return m_offsetTable[finalId + 1] - m_offsetTable[finalId];
  }
  TValueReader const & GetValueReader() const { return m_valueReader; }
  TReader const & GetReader() const { return m_reader; }

private:
  void Parse()
  {
    ReaderSource<TReader> src(m_reader);

    m_huffman.ReadEncoding(src);
    m_numNodes = ReadVarUint<uint32_t, ReaderSource<TReader>>(src);

    BitReader<ReaderSource<TReader>> bitReader(src);
    vector<bool> bv(2 * m_numNodes + 1);
    // Convert the level order representation into the external node representation.
    bv[0] = 1;
    for (size_t i = 0; i < 2 * m_numNodes; ++i)
      bv[i + 1] = bitReader.Read(1) == 1;

    succinct::rs_bit_vector(bv).swap(m_trieTopology);

    uint32_t const numFinalNodes = ReadVarUint<uint32_t, ReaderSource<TReader>>(src);
    m_finalNodeIndex.assign(m_numNodes, -1);
    m_offsetTable.resize(numFinalNodes + 1);
    uint32_t id = 0;
    uint32_t offset = 0;
    for (size_t i = 0; i < numFinalNodes; ++i)
    {
      // ids and offsets are delta-encoded
      id += ReadVarUint<uint32_t, ReaderSource<TReader>>(src);
      offset += ReadVarUint<uint32_t, ReaderSource<TReader>>(src);
      m_finalNodeIndex[id] = i;
      m_offsetTable[i] = offset;
    }
    m_offsetTable[numFinalNodes] = src.Size();
    m_reader = m_reader.SubReader(src.Pos(), src.Size());
  }

  TReader m_reader;

  // todo(@pimenov) Why do we even need an instance? Type name is enough.
  TValueReader const & m_valueReader;
  TEdgeValueReader const & m_edgeValueReader;
  uint32_t m_numNodes;

  coding::HuffmanCoder m_huffman;
  succinct::rs_bit_vector m_trieTopology;

  // m_finalNodeIndex[i] is the 0-based index of the i'th
  // node in the list of all final nodes, or -1 if the
  // node is not final.
  vector<int> m_finalNodeIndex;
  vector<uint32_t> m_offsetTable;
};

template <class TReader, class TValueReader, class TEdgeValueReader>
class SuccinctTrieIterator
{
public:
  using ValueType = typename TValueReader::ValueType;
  using EdgeValueType = typename TEdgeValueReader::ValueType;
  using TCommonData = TopologyAndOffsets<TReader, TValueReader, TEdgeValueReader>;

  SuccinctTrieIterator(TReader const & reader, TCommonData * common, uint32_t nodeBitPosition)
    : m_reader(reader), m_nodeBitPosition(nodeBitPosition), m_valuesRead(false)
  {
    m_common = shared_ptr<TCommonData>(common);
  }

  SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader> * Clone() const
  {
    auto * ret = new SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader>(
        m_reader, m_common.get(), m_nodeBitPosition);
    return ret;
  }

  SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader> * GoToEdge(size_t i) const
  {
    ASSERT(i < 2, ("Bad edge id of a binary trie."));
    ASSERT_EQUAL(m_common->GetTopology()[m_nodeBitPosition - 1], 1, (m_nodeBitPosition));

    // rank(x) returns the number of ones in [0, x) but we count bit positions from 1
    uint32_t childBitPosition = 2 * m_common->GetTopology().rank(m_nodeBitPosition);
    if (i == 1)
      ++childBitPosition;
    if (childBitPosition > 2 * m_common->NumNodes() ||
        m_common->GetTopology()[childBitPosition - 1] == 0)
      return nullptr;
    auto * ret = new SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader>(
        m_reader, m_common.get(), childBitPosition);
    return ret;
  }

  template <typename TEncodingReader>
  SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader> * GoToString(
      TEncodingReader bitEncoding, uint32_t numBits)
  {
    ReaderSource<TEncodingReader> src(bitEncoding);

    // String size. Useful when the string is not alone in the writer to know where it
    // ends but useless for our purpose here.
    ReadVarUint<uint32_t, ReaderSource<TEncodingReader>>(src);

    BitReader<ReaderSource<TEncodingReader>> bitReader(src);

    auto * ret = Clone();
    for (uint32_t i = 0; i < numBits; ++i)
    {
      uint8_t bit = bitReader.Read(1);
      ret = ret->GoToEdge(bit);
      if (!ret)
        return nullptr;
    }
    return ret;
  }

  SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader> * GoToString(
      strings::UniString const & s)
  {
    vector<uint8_t> buf;
    uint32_t numBits;
    {
      MemWriter<vector<uint8_t>> writer(buf);
      numBits = m_common->GetEncoding().EncodeAndWrite(writer, s);
    }
    MemReader reader(&buf[0], buf.size());
    return GoToString(reader, numBits);
  }

  uint32_t NumValues()
  {
    if (!m_valuesRead)
      ReadValues();
    return m_values.size();
  }

  ValueType GetValue(size_t i)
  {
    if (!m_valuesRead)
      ReadValues();
    ASSERT(i < m_values.size(), ());
    return m_values[i];
  }

private:
  void ReadValues()
  {
    if (m_valuesRead)
      return;
    m_valuesRead = true;
    // Back to 0-based indices.
    uint32_t m_nodeId = m_common->GetTopology().rank(m_nodeBitPosition) - 1;
    if (!m_common->NodeIsFinal(m_nodeId))
      return;
    uint32_t offset = m_common->Offset(m_nodeId);
    uint32_t size = m_common->ValueListSize(m_nodeId);
    ReaderPtr<TReader> subReaderPtr(m_reader.CreateSubReader(offset, size));
    ReaderSource<ReaderPtr<TReader>> src(subReaderPtr);
    while (src.Size() > 0)
    {
      ValueType val;
      m_common->GetValueReader()(src, val);
      m_values.push_back(val);
    }
  }

  TReader const & m_reader;
  shared_ptr<TCommonData> m_common;

  // The bit with this 1-based index represents this node
  // in the external node representation of binary trie.
  uint32_t m_nodeBitPosition;

  vector<ValueType> m_values;
  bool m_valuesRead;
};

template <typename TReader, typename TValueReader, typename TEdgeValueReader>
SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader> * ReadSuccinctTrie(
    TReader const & reader, TValueReader valueReader = TValueReader(),
    TEdgeValueReader edgeValueReader = TEdgeValueReader())
{
  auto * common = new TopologyAndOffsets<TReader, TValueReader, TEdgeValueReader>(
      reader, valueReader, edgeValueReader);
  return new SuccinctTrieIterator<TReader, TValueReader, TEdgeValueReader>(
      common->GetReader(), common, 1 /* bitPosition */);
}

}  // namespace trie
