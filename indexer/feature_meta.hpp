#pragma once

#include "coding/reader.hpp"
#include "coding/multilang_utf8_string.hpp"

#include "std/map.hpp"
#include "std/string.hpp"
#include "std/limits.hpp"
#include "std/algorithm.hpp"
#include "std/vector.hpp"

namespace feature
{
  class Metadata
  {
    map<uint8_t, string> m_metadata;

  public:
    enum EType
    {
      FMD_CUISINE = 1,
      FMD_OPEN_HOURS = 2,
      FMD_PHONE_NUMBER = 3,
      FMD_FAX_NUMBER = 4,
      FMD_STARS = 5,
      FMD_OPERATOR = 6,
      FMD_URL = 7,
      FMD_WEBSITE = 8,
      FMD_INTERNET = 9,
      FMD_ELE = 10,
      FMD_TURN_LANES = 11,
      FMD_TURN_LANES_FORWARD = 12,
      FMD_TURN_LANES_BACKWARD = 13,
      FMD_EMAIL = 14,
      FMD_POSTCODE = 15
    };

    bool Add(EType type, string const & s)
    {
      string & val = m_metadata[type];
      if (val.empty())
        val = s;
      else
        val = val + ", " + s;
      return true;
    }

    string Get(EType type) const
    {
      auto it = m_metadata.find(type);
      return (it == m_metadata.end()) ? string() : it->second;
    }

    vector<EType> GetPresentTypes() const
    {
      vector<EType> types;
      for (auto item : m_metadata)
        types.push_back(static_cast<EType>(item.first));

      return types;
    }

    void Drop(EType type)
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
        uint8_t elem[2] = {static_cast<uint8_t>(e.first | last_key_mark),
                           static_cast<uint8_t>(min(e.second.size(), (size_t)numeric_limits<uint8_t>::max()))};
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
      uint8_t const sz = m_metadata.size();
      WriteToSink(ar, sz);
      if (sz)
      {
        for (auto const & it : m_metadata)
        {
          WriteToSink(ar, static_cast<uint8_t>(it.first));
          utils::WriteString(ar, it.second);
        }
      }
    }

    template <class ArchiveT> void Deserialize(ArchiveT & ar)
    {
      uint8_t const sz = ReadPrimitiveFromSource<uint8_t>(ar);
      for (size_t i = 0; i < sz; ++i)
      {
        uint8_t const key = ReadPrimitiveFromSource<uint8_t>(ar);
        string value;
        utils::ReadString(ar, value);
        m_metadata.insert(make_pair(key, value));
      }
    }
  };
}
