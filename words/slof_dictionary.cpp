#include "slof_dictionary.hpp"
#include "../coding/bzip2_compressor.hpp"
#include "../coding/byte_stream.hpp"
#include "../coding/endianness.hpp"
#include "../coding/reader.hpp"
#include "../coding/varint.hpp"
#include "../base/logging.hpp"
#include "../base/base.hpp"
#include "../std/utility.hpp"

namespace
{
  char const * SkipVarUint(char const * p)
  {
    while ((*p) & 128)
      ++p;
    return ++p;
  }
}

sl::SlofDictionary::SlofDictionary(Reader const * pReader)
      : m_pReader(pReader), m_Decompressor(&DecompressBZip2IntoFixedSize)
{
  Init();
}

sl::SlofDictionary::SlofDictionary(
    Reader const * pReader, function<void (const char *, size_t, char *, size_t)> decompressor)
      : m_pReader(pReader), m_Decompressor(decompressor)
{
  Init();
}

void sl::SlofDictionary::Init()
{
  m_pReader->Read(0, &m_Header, sizeof(m_Header));
  if (m_Header.m_MajorVersion != 1)
    MYTHROW(OpenDictionaryNewerVersionException, (m_Header.m_MajorVersion));
}

sl::SlofDictionary::~SlofDictionary()
{
}

sl::Dictionary::Id sl::SlofDictionary::KeyCount() const
{
  return m_Header.m_KeyCount;
}

void sl::SlofDictionary::ReadKeyData(sl::Dictionary::Id id, string & data) const
{
  pair<uint32_t, uint32_t> offsets;
  m_pReader->Read(m_Header.m_KeyIndexOffset + id * 4, &offsets, 8);
  offsets.first = SwapIfBigEndian(offsets.first);
  offsets.second = SwapIfBigEndian(offsets.second);
  // Add 2 trailing zeroes, to be sure that reading varint doesn't hang if file is corrupted.
  data.resize(offsets.second - offsets.first + 2);
  m_pReader->Read(m_Header.m_KeyDataOffset + offsets.first,
                  &data[0], offsets.second - offsets.first);
}

void sl::SlofDictionary::KeyById(sl::Dictionary::Id id, string & key) const
{
  string keyData;
  ReadKeyData(id, keyData);
  char const * pBeg = SkipVarUint(SkipVarUint(&keyData[0]));
  char const * pLast = &keyData[keyData.size() - 1];
  // ReadKeyData adds trailing zeroes, so that reading VarUint doesn't hang up in case of
  // corrupted data. Strip them.
  while (pLast >= pBeg && *pLast == 0)
    --pLast;
  key.assign(pBeg, pLast + 1);
}

void sl::SlofDictionary::ArticleById(sl::Dictionary::Id id, string & article) const
{
  string keyData;
  ReadKeyData(id, keyData);
  ArrayByteSource keyDataSource(&keyData[0]);
  uint64_t const articleChunkPos = ReadVarUint<uint64_t>(keyDataSource);
  uint32_t const articleNumInChunk = ReadVarUint<uint32_t>(keyDataSource);

  uint32_t const chunkSize =
      ReadPrimitiveFromPos<uint32_t>(*m_pReader, m_Header.m_ArticleOffset + articleChunkPos);
  string chunk(chunkSize, 0);
  m_pReader->Read(m_Header.m_ArticleOffset + articleChunkPos + 4, &chunk[0], chunkSize);
  ArrayByteSource chunkSource(&chunk[0]);
  uint32_t chunkHeaderSize = ReadVarUint<uint32_t>(chunkSource);
  chunkHeaderSize += static_cast<char const *>(chunkSource.Ptr()) - &chunk[0];
  uint32_t const decompressedChunkSize = ReadVarUint<uint32_t>(chunkSource);
  uint32_t articleBegInChunk = 0;
  for (uint32_t i = 0; i < articleNumInChunk; ++i)
    articleBegInChunk += ReadVarUint<uint32_t>(chunkSource);
  uint32_t const articleSizeInChunk = ReadVarUint<uint32_t>(chunkSource);
  vector<char> decompressedChunk(decompressedChunkSize);
  m_Decompressor(&chunk[chunkHeaderSize], chunkSize - chunkHeaderSize,
                 &decompressedChunk[0], decompressedChunkSize);
  article.assign(&decompressedChunk[articleBegInChunk], articleSizeInChunk);
}
