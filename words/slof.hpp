#pragma once
// slof = sloynik format.
//
// VU = VarUint.
//
// Offset:               Format:
//
// 0                     [Header]
// m_ArticleOffset       [VU ChunkSize] [Compressed article chunk 0]
//                       ..
//                       [VU ChunkSize] [Compressed article chunk n-1]
// m_KeyDataOffset       [VU KeySize] [Key 0]
//                       ..
//                       [VU KeySize] [Key m_KeyCount-1]
// m_KeyArticleIdOffset  [VU article 0 chunk offset]                [VU article 0 number in chunk]
//                       ..
//                       [VU article m_ArticleCount-1 chunk offset] [VU article * number in chunk]
//
// ArticleId = ([Offset of article chunk] << 24) + [Offset of article in uncompressed chunk]

#include "../coding/endianness.hpp"
#include "../base/base.hpp"

namespace sl
{

#pragma pack(push, 1)
struct SlofHeader
{
  uint32_t m_Signature;           // = "slof"
  uint16_t m_MajorVersion;        // Major version. Changing it breaks compatibility.
  uint16_t m_MinorVersion;        // Minor version. Changing it doesn't break compatibility.

  uint32_t m_KeyCount;            // Number of keys.
  uint32_t m_ArticleCount;        // Number of articles.

  uint64_t m_KeyIndexOffset;      // Offset of key cummulative lengthes.
  uint64_t m_KeyDataOffset;       // Offset of key data.
  uint64_t m_ArticleOffset;       // Offset of article data.
};
#pragma pack(pop)

}
