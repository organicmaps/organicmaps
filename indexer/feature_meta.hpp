#pragma once

#include "../coding/reader.hpp"
#include "../coding/multilang_utf8_string.hpp"

#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/limits.hpp"
#include "../std/algorithm.hpp"

namespace feature
{
  class FeatureMetadata
  {
    typedef map<uint8_t, string> MetadataT;
    MetadataT m_metadata;

  public:
    enum EMetadataType {
      FMD_CUISINE = 1,
      FMD_OPEN_HOURS,
      FMD_PHONE_NUMBER,
      FMD_FAX_NUMBER,
      FMD_STARS,
      FMD_OPERATOR,
      FMD_URL,
      FMD_INTERNET
    };

    bool Add(EMetadataType type, string const & s)
    {
      if (m_metadata[type].empty())
      {
        m_metadata[type] = s;
      }
      else
      {
        m_metadata[type] = m_metadata[type] + ", " + s;
      }
      return true;
    }

    string Get(EMetadataType type) const
    {
      auto it = m_metadata.find(type);
      return (it == m_metadata.end()) ? string() : it->second;
    }

    void Drop(EMetadataType type)
    {
      m_metadata.erase(type);
    }

    inline bool Empty() const { return m_metadata.empty(); }
    inline size_t Size() const { return m_metadata.size(); }

    template <class ArchiveT> void SerializeToMWM(ArchiveT & ar) const
    {
      for (auto const & e: m_metadata)
      {
        uint8_t last_key_mark = (&e == &(*m_metadata.crbegin())) << 7; /// set high bit (0x80) if it last element
        uint8_t elem[2] = {static_cast<uint8_t>(e.first | last_key_mark), static_cast<uint8_t>(min(e.second.size(), (size_t)numeric_limits<uint8_t>::max()))};
        ar.Write(elem, sizeof(elem));
        ar.Write(e.second.data(), elem[1]);
      }
    }
    template <class ArchiveT> void DeserializeFromMWM(ArchiveT & ar)
    {
      uint8_t header[2] = {0};
      char buffer[uint8_t(-1)] = {0};
      do
      {
        ar.Read(header, sizeof(header));
        ar.Read(buffer, header[1]);
        m_metadata[static_cast<uint8_t>(header[0] & 0x7F)].assign(buffer, header[1]);
      } while (!(header[0] & 0x80));
    }

    template <class ArchiveT> void Serialize(ArchiveT & ar) const
    {
      uint8_t const metadata_size = m_metadata.size();
      WriteToSink(ar, metadata_size);
      if (metadata_size)
      {
        for(auto & it: m_metadata)
        {
          WriteToSink(ar, static_cast<uint8_t>(it.first));
          utils::WriteString(ar, it.second);
        }
      }
    }

    template <class ArchiveT> void Deserialize(ArchiveT & ar)
    {
      uint8_t const metadata_size = ReadPrimitiveFromSource<uint8_t>(ar);
      for (size_t i=0; i < metadata_size; ++i)
      {
        uint8_t const key = ReadPrimitiveFromSource<uint8_t>(ar);
        string value;
        utils::ReadString(ar, value);
        m_metadata.insert(make_pair(key, value));
      }

    }
  };
}
