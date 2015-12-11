#pragma once

#include "coding/multilang_utf8_string.hpp"
#include "coding/reader.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"


namespace feature
{
  class MetadataBase
  {
  protected:
    void Set(uint8_t type, string const & value)
    {
      auto found = m_metadata.find(type);
      if (found == m_metadata.end())
      {
        if (!value.empty())
          m_metadata[type] = value;
      }
      else
      {
        if (value.empty())
          m_metadata.erase(found);
        else
          found->second = value;
      }
    }

  public:
    string Get(uint8_t type) const
    {
      auto it = m_metadata.find(type);
      return (it == m_metadata.end()) ? string() : it->second;
    }

    vector<uint8_t> GetPresentTypes() const
    {
      vector<uint8_t> types;
      types.reserve(m_metadata.size());

      for (auto const & item : m_metadata)
        types.push_back(item.first);

      return types;
    }

    inline bool Empty() const { return m_metadata.empty(); }
    inline size_t Size() const { return m_metadata.size(); }

    template <class TSink> void Serialize(TSink & sink) const
    {
      uint8_t const sz = m_metadata.size();
      WriteToSink(sink, sz);
      for (auto const & it : m_metadata)
      {
        WriteToSink(sink, static_cast<uint8_t>(it.first));
        utils::WriteString(sink, it.second);
      }
    }

    template <class TSource> void Deserialize(TSource & src)
    {
      uint8_t const sz = ReadPrimitiveFromSource<uint8_t>(src);
      for (size_t i = 0; i < sz; ++i)
      {
        uint8_t const key = ReadPrimitiveFromSource<uint8_t>(src);
        string value;
        utils::ReadString(src, value);
        m_metadata[key].swap(value);
      }
    }

  protected:
    map<uint8_t, string> m_metadata;
  };

  class Metadata : public MetadataBase
  {
  public:
    /// @note! Do not change values here.
    /// Add new types to the end of list, before FMD_COUNT.
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
      FMD_POSTCODE = 15,
      FMD_WIKIPEDIA = 16,
      FMD_MAXSPEED = 17,
      FMD_FLATS = 18,
      FMD_HEIGHT = 19,
      FMD_MIN_HEIGHT = 20,
      FMD_DENOMINATION = 21,
      FMD_COUNT
    };

    static_assert(FMD_COUNT <= 255, "Meta types count is limited to one byte.");

    void Set(EType type, string const & value)
    {
      MetadataBase::Set(type, value);
    }

    void Drop(EType type) { Set(type, string()); }

    string GetWikiURL() const;

    template <class TWriter> void SerializeToMWM(TWriter & writer) const
    {
      for (auto const & e : m_metadata)
      {
        // set high bit if it's the last element
        uint8_t const mark = (&e == &(*m_metadata.crbegin()) ? 0x80 : 0);
        uint8_t elem[2] = {static_cast<uint8_t>(e.first | mark),
                           static_cast<uint8_t>(min(e.second.size(), (size_t)kMaxStringLength))};
        writer.Write(elem, sizeof(elem));
        writer.Write(e.second.data(), elem[1]);
      }
    }

    template <class TSource> void DeserializeFromMWM(TSource & src)
    {
      uint8_t header[2] = {0};
      char buffer[kMaxStringLength] = {0};
      do
      {
        src.Read(header, sizeof(header));
        src.Read(buffer, header[1]);
        m_metadata[header[0] & 0x7F].assign(buffer, header[1]);
      } while (!(header[0] & 0x80));
    }

  private:
    enum { kMaxStringLength = 255 };
  };

  class AddressData : public MetadataBase
  {
  public:
    enum Type { PLACE, STREET, POSTCODE };

    void Add(Type type, string const & s)
    {
      /// @todo Probably, we need to add separator here and store multiple values.
      MetadataBase::Set(type, s);
    }
  };
}
