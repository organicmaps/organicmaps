#pragma once
#include "coding/bit_streams.hpp"
#include "coding/huffman.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "3party/succinct/rs_bit_vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace trie
{
// This class is used to read the data that is common to every node
// of the trie: the trie topology and the offsets into the data buffer.
// The topology can then be used to navigate the trie and the offsets
// can be used to extract the values associated with the key strings.
template <class TReader, class TValueReader>
class TopologyAndOffsets
{
public:
  using TValue = typename TValueReader::ValueType;

  TopologyAndOffsets(TReader const & reader, TValueReader const & valueReader)
    : m_reader(reader)
    , m_valueReader(valueReader)
  {
    Parse();
  }

  // Returns the topology of the trie in the external node representation.
  // Arrange the nodes in the level order and write two bits for every
  // node: the first bit for its left child and the second bit for its right child.
  // Write '1' if the child is present and '0' if it is not.
  // Prepend the resulting bit string with '1' (for a super-root) and you will get
  // the external node representaiton.
  succinct::rs_bit_vector const & GetTopology() const { return m_trieTopology; }

  // Returns the number of trie nodes.
  uint32_t NumNodes() const { return m_numNodes; }

  // Returns the Huffman encoding that was used to encode the strings
  // before adding them to this trie.
  coding::HuffmanCoder const & GetEncoding() const { return m_huffman; }

  // Returns true if and only if the node is the last node of a string
  // that was used as a key when building this trie.
  bool NodeIsFinal(uint32_t id) const { return m_finalNodeIndex[id] >= 0; }

  // Returns the offset relative to the beginning of the start of the
  // value buffer where the data for node number |id| starts.
  uint32_t Offset(uint32_t id) const
  {
    CHECK(NodeIsFinal(id), ());
    return m_offsetTable[m_finalNodeIndex[id]];
  }

  // Returns the number of values associated with a final node |id|.
  uint32_t ValueListSize(uint32_t id)
  {
    CHECK(NodeIsFinal(id), ());
    int finalId = m_finalNodeIndex[id];
    return m_offsetTable[finalId + 1] - m_offsetTable[finalId];
  }

  // Returns the reader from which the value buffer can be read.
  TValueReader const & GetValueReader() const { return m_valueReader; }

  // Returns the reader from which the trie can be read.
  TReader const & GetReader() const { return m_reader; }

private:
  void Parse()
  {
    ReaderSource<TReader> src(m_reader);

    m_huffman.ReadEncoding(src);
    m_numNodes = ReadVarUint<uint32_t, ReaderSource<TReader>>(src);

    BitReader<ReaderSource<TReader>> bitReader(src);
    std::vector<bool> bv(2 * m_numNodes + 1);
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
      m_finalNodeIndex[id] = base::asserted_cast<int>(i);
      m_offsetTable[i] = offset;
    }
    m_offsetTable[numFinalNodes] = base::checked_cast<uint32_t>(src.Size());
    m_reader = m_reader.SubReader(src.Pos(), src.Size());
  }

  TReader m_reader;

  // todo(@pimenov) Why do we even need an instance? Type name is enough.
  TValueReader const & m_valueReader;
  uint32_t m_numNodes;

  coding::HuffmanCoder m_huffman;
  succinct::rs_bit_vector m_trieTopology;

  // m_finalNodeIndex[i] is the 0-based index of the i'th
  // node in the list of all final nodes, or -1 if the
  // node is not final.
  std::vector<int> m_finalNodeIndex;
  std::vector<uint32_t> m_offsetTable;
};

template <class TReader, class TValueReader>
class SuccinctTrieIterator
{
public:
  using TValue = typename TValueReader::ValueType;
  using TCommonData = TopologyAndOffsets<TReader, TValueReader>;

  SuccinctTrieIterator(TReader const & reader, std::shared_ptr<TCommonData> common, uint32_t nodeBitPosition)
    : m_reader(reader)
    , m_common(common)
    , m_nodeBitPosition(nodeBitPosition)
    , m_valuesRead(false)
  {}

  std::unique_ptr<SuccinctTrieIterator> Clone() const
  {
    return std::make_unique<SuccinctTrieIterator>(m_reader, m_common, m_nodeBitPosition);
  }

  std::unique_ptr<SuccinctTrieIterator> GoToEdge(size_t i) const
  {
    ASSERT_LESS(i, 2, ("Bad edge id of a binary trie."));
    ASSERT_EQUAL(m_common->GetTopology()[m_nodeBitPosition - 1], 1, (m_nodeBitPosition));

    // rank(x) returns the number of ones in [0, x) but we count bit positions from 1
    uint32_t childBitPosition = base::asserted_cast<uint32_t>(2 * m_common->GetTopology().rank(m_nodeBitPosition));
    if (i == 1)
      ++childBitPosition;
    if (childBitPosition > 2 * m_common->NumNodes() || m_common->GetTopology()[childBitPosition - 1] == 0)
      return nullptr;
    return std::make_unique<SuccinctTrieIterator>(m_reader, m_common, childBitPosition);
  }

  template <typename TEncodingReader>
  std::unique_ptr<SuccinctTrieIterator> GoToString(TEncodingReader & bitEncoding, uint32_t numBits)
  {
    ReaderSource<TEncodingReader> src(bitEncoding);

    // String size. Useful when the string is not alone in the writer to know where it
    // ends but useless for our purpose here.
    ReadVarUint<uint32_t, ReaderSource<TEncodingReader>>(src);

    BitReader<ReaderSource<TEncodingReader>> bitReader(src);

    auto ret = Clone();
    for (uint32_t i = 0; i < numBits; ++i)
    {
      uint8_t bit = bitReader.Read(1);
      auto nxt = ret->GoToEdge(bit);
      if (!nxt)
        return nullptr;
      ret = std::move(nxt);
    }
    return ret;
  }

  std::unique_ptr<SuccinctTrieIterator> GoToString(strings::UniString const & s)
  {
    std::vector<uint8_t> buf;
    uint32_t numBits;
    {
      MemWriter<std::vector<uint8_t>> writer(buf);
      numBits = m_common->GetEncoding().EncodeAndWrite(writer, s);
    }
    MemReader reader(&buf[0], buf.size());
    return GoToString(reader, numBits);
  }

  size_t NumValues()
  {
    if (!m_valuesRead)
      ReadValues();
    return m_values.size();
  }

  TValue GetValue(size_t i)
  {
    if (!m_valuesRead)
      ReadValues();
    ASSERT_LESS(i, m_values.size(), ());
    return m_values[i];
  }

private:
  void ReadValues()
  {
    if (m_valuesRead)
      return;
    m_valuesRead = true;
    // Back to 0-based indices.
    uint32_t m_nodeId = base::checked_cast<uint32_t>(m_common->GetTopology().rank(m_nodeBitPosition) - 1);
    if (!m_common->NodeIsFinal(m_nodeId))
      return;
    uint32_t offset = m_common->Offset(m_nodeId);
    uint32_t size = m_common->ValueListSize(m_nodeId);
    ReaderPtr<TReader> subReaderPtr(
        std::unique_ptr<TReader>(static_cast<TReader *>(m_reader.CreateSubReader(offset, size).release())));
    ReaderSource<ReaderPtr<TReader>> src(subReaderPtr);
    while (src.Size() > 0)
    {
      TValue val;
      m_common->GetValueReader()(src, val);
      m_values.push_back(val);
    }
  }

  TReader const & m_reader;
  std::shared_ptr<TCommonData> m_common;

  // The bit with this 1-based index represents this node
  // in the external node representation of binary trie.
  uint32_t m_nodeBitPosition;

  std::vector<TValue> m_values;
  bool m_valuesRead;
};

template <typename TReader, typename TValueReader>
std::unique_ptr<SuccinctTrieIterator<TReader, TValueReader>> ReadSuccinctTrie(TReader const & reader,
                                                                              TValueReader valueReader = TValueReader())
{
  using TCommonData = TopologyAndOffsets<TReader, TValueReader>;
  using TIter = SuccinctTrieIterator<TReader, TValueReader>;

  std::shared_ptr<TCommonData> common(new TCommonData(reader, valueReader));
  return std::make_unique<TIter>(common->GetReader(), common, 1 /* bitPosition */);
}

}  // namespace trie
