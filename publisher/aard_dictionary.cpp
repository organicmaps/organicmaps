#include "aard_dictionary.hpp"
#include "../coding_sloynik/bzip2_compressor.hpp"
#include "../coding_sloynik/gzip_compressor.hpp"
#include "../coding/endianness.hpp"
#include "../coding/reader.hpp"
#include "../base/logging.hpp"
#include "../3party/jansson/myjansson.hpp"
#include "../std/exception.hpp"

namespace
{
  template <typename T> inline T SwapIfLittleEndian(T t)
  {
  #ifdef ENDIAN_IS_BIG
    return t;
  #else
    return ReverseByteOrder(t);
  #endif
  }
  template <typename PrimitiveT, class TReader>
  PrimitiveT ReadLittleEndianPrimitiveFromPos(TReader & reader, uint64_t pos)
  {
    PrimitiveT primitive;
    ReadFromPos(reader, pos, &primitive, sizeof(primitive));
    return SwapIfLittleEndian(primitive);
  }

}

namespace sl
{

enum
{
  AARD_DICTIONARY_MAX_ARTICLE_SIZE = 1 << 24
};

#pragma pack(push, 1)
struct AardDictionaryIndex1Item
{
  uint32_t m_KeyPos;
  uint32_t m_ArticlePos;
};

struct AardDictionary::AardDictionaryHeader
{
  char m_Signature[4];
  uint8_t m_Sha1[40];
  uint16_t m_Version;
  uint8_t m_Uuid[16];
  uint16_t m_Volume;
  uint16_t m_TotalVolumes;
  uint32_t m_MetaLength;
  uint32_t m_IndexCount;
  uint32_t m_ArticleOffset;
  char m_Index1ItemFormat[4];
  char m_KeyLengthFormat[2];
  char m_ArticleLengthFormat[2];

  // Offset of the index1 items.
  uint32_t Index1Offset() const
  {
    return sizeof(AardDictionaryHeader) + m_MetaLength;
  }
  // Offset of the keys data.
  uint32_t KeyDataOffset() const
  {
    return Index1Offset() + sizeof(AardDictionaryIndex1Item) * m_IndexCount;
  }
  // Offset of the article data.
  uint32_t ArticleOffset() const
  {
    return m_ArticleOffset;
  }
};
#pragma pack(pop)

void AardDecompress(char const * pSrc, size_t srcSize, string & dst)
{
  bool decompressed = false;
  try
  {
    DecompressGZip(pSrc, srcSize, dst);
    decompressed = true;
  }
  catch (StringCodingException &) {}
  if (decompressed)
    return;
  try
  {
    DecompressBZip2(pSrc, srcSize, dst);
    decompressed = true;
  }
  catch (StringCodingException &) {}
  if (decompressed)
    return;
  dst.assign(pSrc, pSrc + srcSize);
}

AardDictionary::AardDictionary(Reader const & reader)
  : m_Reader(reader), m_pHeader(new AardDictionaryHeader)
{
  m_Reader.Read(0, m_pHeader.get(), sizeof(AardDictionaryHeader));
  m_pHeader->m_Volume = SwapIfLittleEndian(m_pHeader->m_Volume);
  m_pHeader->m_TotalVolumes = SwapIfLittleEndian(m_pHeader->m_TotalVolumes);
  m_pHeader->m_MetaLength = SwapIfLittleEndian(m_pHeader->m_MetaLength);
  m_pHeader->m_IndexCount = SwapIfLittleEndian(m_pHeader->m_IndexCount);
  m_pHeader->m_ArticleOffset = SwapIfLittleEndian(m_pHeader->m_ArticleOffset);
  if (memcmp(m_pHeader->m_Signature, "aard", 4) != 0)
    MYTHROW(Dictionary::OpenDictionaryException, ("Invalid signature."));
  if (memcmp(m_pHeader->m_Index1ItemFormat, ">LL\0", 4) != 0)
    MYTHROW(Dictionary::OpenDictionaryException, ("Invalid index1 item format."));
  if (memcmp(m_pHeader->m_KeyLengthFormat, ">H", 2) != 0)
    MYTHROW(Dictionary::OpenDictionaryException, ("Invalid key length format."));
  if (memcmp(m_pHeader->m_ArticleLengthFormat, ">L", 2) != 0)
    MYTHROW(Dictionary::OpenDictionaryException, ("Invalid article length format."));
  LOG(LINFO, ("Loaded aard dictionary, volume:", m_pHeader->m_Volume,
              "of", m_pHeader->m_TotalVolumes,
              "meta length:", m_pHeader->m_MetaLength,
              "words:", m_pHeader->m_IndexCount));

  // TODO: What to do with duplicate keys?
  for (Id i = 0; i < KeyCount(); ++i)
  {
    string key;
    KeyById(i, key);
    m_KeyToIdMap[key] = i;
  }
}

AardDictionary::~AardDictionary()
{
}

sl::Dictionary::Id AardDictionary::KeyCount() const
{
  return m_pHeader->m_IndexCount;
}

void AardDictionary::KeyById(Id id, string & key) const
{
  AardDictionaryIndex1Item item;
  m_Reader.Read(m_pHeader->Index1Offset() + id * sizeof(item), &item, sizeof(item));
  uint64_t const keyPos = m_pHeader->KeyDataOffset() + SwapIfLittleEndian(item.m_KeyPos);
  uint16_t const keyLength = ReadLittleEndianPrimitiveFromPos<uint16_t>(m_Reader, keyPos);
  if (keyLength == 0)
    MYTHROW(Dictionary::BrokenDictionaryException, (keyLength));
  key.resize(keyLength);
  m_Reader.Read(keyPos + 2, &key[0], keyLength);
}

void AardDictionary::ArticleById(Id id, string & article) const
{
  string key;
  KeyById(id, key);

  AardDictionaryIndex1Item item;
  m_Reader.Read(m_pHeader->Index1Offset() + id * sizeof(item), &item, sizeof(item));
  uint64_t const articlePos = m_pHeader->ArticleOffset() + SwapIfLittleEndian(item.m_ArticlePos);
  uint32_t articleLength = ReadLittleEndianPrimitiveFromPos<uint32_t>(m_Reader, articlePos);
  if (articleLength > AARD_DICTIONARY_MAX_ARTICLE_SIZE)
    MYTHROW(BrokenDictionaryException, (articleLength));
  CHECK_NOT_EQUAL(articleLength, 0, ());
  try
  {
    string compressedArticle(articleLength, '.');
    m_Reader.Read(articlePos + 4, &compressedArticle[0], articleLength);
    string articleJSON;
    AardDecompress(&compressedArticle[0], compressedArticle.size(), articleJSON);

    my::Json root(articleJSON.c_str());
    CHECK_EQUAL(json_typeof(root), JSON_ARRAY, (id, key));
    json_t * pJsonElement0(json_array_get(root, 0));
    CHECK(pJsonElement0, (id, key));
    CHECK_EQUAL(json_typeof(pJsonElement0), JSON_STRING, (id, key));
    string s0 = json_string_value(pJsonElement0);
    if (s0.size() > 0)
    {
      // Normal article.
      for (unsigned int i = 1; i < json_array_size(root); ++i)
      {
        json_t * pJsonElementI = json_array_get(root, i);
        CHECK(pJsonElementI, (id, key));
        switch (json_type jsonElementIType = json_typeof(pJsonElementI))
        {
        case JSON_ARRAY:
          CHECK_EQUAL(json_array_size(pJsonElementI), 0, (id, key, articleJSON));
          break;
        case JSON_OBJECT:
          CHECK_EQUAL(json_object_size(pJsonElementI), 0, (id, key, articleJSON));
          break;
        default:
          CHECK(false, (id, key, jsonElementIType, articleJSON));
        }
      }
      article.swap(s0);
    }
    else
    {
      // Redirect
      CHECK_EQUAL(json_array_size(root), 3, (id, key));
      json_t * pJsonElement2(json_array_get(root, 2));
      CHECK_EQUAL(json_typeof(pJsonElement2), JSON_OBJECT, (id, key));
      CHECK_EQUAL(json_object_size(pJsonElement2), 1, (id, key));
      json_t * pJsonRedirect = json_object_get(pJsonElement2, "r");
      CHECK(pJsonRedirect, (id, key));
      CHECK_EQUAL(json_typeof(pJsonRedirect), JSON_STRING, (id, key));
      string redirectStr(json_string_value(pJsonRedirect));
      CHECK_GREATER(redirectStr.size(), 0, (id, key));
      map<string, Id>::const_iterator it = m_KeyToIdMap.find(redirectStr);
      if (it == m_KeyToIdMap.end())
      {
        LOG(LWARNING, ("Incorrect redirect", id, key, redirectStr));
        return;
      }
      ArticleById(it->second, article);
    }
  }
  catch (exception & e)
  {
    CHECK(false, (id, key, e.what()));
  }
}

}
