// See O5M Format definition at https://wiki.openstreetmap.org/wiki/O5m
#pragma once

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <vector>

namespace osm
{
using TReadFunc = std::function<size_t(uint8_t *, size_t)>;

class StreamBuffer
{
  using TBuffer = std::vector<uint8_t>;

  TReadFunc m_reader;
  TBuffer m_buffer;
  size_t const m_maxBufferSize;
  size_t m_recap;  // recap read bytes

  TBuffer::const_iterator m_position;

public:
  StreamBuffer(TReadFunc reader, size_t readBufferSizeInBytes)
    : m_reader(reader)
    , m_buffer(readBufferSizeInBytes)
    , m_maxBufferSize(readBufferSizeInBytes)
    , m_recap(0)
  {
    Refill();
  }

  size_t Recap()
  {
    size_t const recap = m_recap;
    m_recap = 0;
    return recap;
  }

  inline TBuffer::value_type Get()
  {
    if (m_position == m_buffer.end())
      Refill();
    ++m_recap;
    return *(m_position++);
  }

  inline TBuffer::value_type Peek()
  {
    if (m_position == m_buffer.end())
      Refill();
    return *m_position;
  }

  void Skip(size_t size = 1)
  {
    size_t const bytesLeft = std::distance(m_position, m_buffer.cend());
    if (size >= bytesLeft)
    {
      size -= bytesLeft;
      for (Refill(); size > m_buffer.size(); size -= m_buffer.size())
        Refill();
    }
    m_position += size;
    m_recap += size;
  }

  void Read(TBuffer::value_type * dest, size_t size)
  {
    size_t const bytesLeft = std::distance(m_position, m_buffer.cend());
    if (size >= bytesLeft)
    {
      size_t const index = std::distance(m_buffer.cbegin(), m_position);
      memmove(dest, &m_buffer[index], bytesLeft);
      size -= bytesLeft;
      dest += bytesLeft;
      for (Refill(); size > m_buffer.size(); size -= m_buffer.size())
      {
        memmove(dest, m_buffer.data(), m_buffer.size());
        dest += m_buffer.size();
        Refill();
      }
    }
    size_t const index = std::distance(m_buffer.cbegin(), m_position);
    memmove(dest, &m_buffer[index], size);
    m_position += size;
  }

private:
  void Refill()
  {
    if (m_buffer.size() != m_maxBufferSize)
      m_buffer.resize(m_maxBufferSize);

    size_t const readBytes = m_reader(m_buffer.data(), m_buffer.size());
    CHECK_NOT_EQUAL(readBytes, 0, ("Unexpected std::end input stream."));

    if (readBytes != m_buffer.size())
      m_buffer.resize(readBytes);

    m_position = m_buffer.begin();
  }
};

class O5MSource
{
public:
  enum class EntityType
  {
    End = 0xfe,
    Node = 0x10,
    Way = 0x11,
    Relation = 0x12,
    BBox = 0xdb,
    Timestamp = 0xdc,
    Header = 0xe0,
    Sync = 0xee,
    Jump = 0xef,
    Reset = 0xff
  };

  friend std::ostream & operator<<(std::ostream & s, EntityType const & type)
  {
    switch (type)
    {
    case EntityType::End: s << "O5M_CMD_END"; break;
    case EntityType::Node: s << "O5M_CMD_NODE"; break;
    case EntityType::Way: s << "O5M_CMD_WAY"; break;
    case EntityType::Relation: s << "O5M_CMD_REL"; break;
    case EntityType::BBox: s << "O5M_CMD_BBOX"; break;
    case EntityType::Timestamp: s << "O5M_CMD_TSTAMP"; break;
    case EntityType::Header: s << "O5M_CMD_HEADER"; break;
    case EntityType::Sync: s << "O5M_CMD_SYNC"; break;
    case EntityType::Jump: s << "O5M_CMD_JUMP"; break;
    case EntityType::Reset: s << "O5M_CMD_RESET"; break;
    default: return s << "Unknown command: " << std::hex << base::Underlying(type);
    }
    return s;
  }

protected:
  struct StringTableRecord
  {
    // This important value got from
    // documentation ( https://wiki.openstreetmap.org/wiki/O5m#Strings ) on O5M format.
    // If change it all will be broken.
    enum
    {
      MaxEntrySize = 252
    };

    char key[MaxEntrySize];
    char value[MaxEntrySize];
  };

  struct KeyValue
  {
    char const * key = nullptr;
    char const * value = nullptr;

    KeyValue() = default;
    KeyValue(char const * k, char const * v) : key(k), value(v) {}
  };

  struct Member
  {
    int64_t ref = 0;
    EntityType type = EntityType::Reset;
    char const * role = nullptr;
  };

  std::vector<StringTableRecord> m_stringTable;
  std::vector<char> m_stringBuffer;
  size_t m_stringCurrentIndex;
  StreamBuffer m_buffer;
  size_t m_remainder = 0;
  int64_t m_currentNodeRef = 0;
  int64_t m_currentWayRef = 0;
  int64_t m_currentRelationRef = 0;
  int64_t m_id = 0;
  int32_t m_lon = 0;
  int32_t m_lat = 0;
  uint64_t m_timestamp = 0;
  uint64_t m_changeset = 0;
  int64_t m_middlePartSize = 0;  // Length of the references section

public:
  template <typename TValue>
  class SubElements
  {
    using TSubElementGetter = std::function<O5MSource *(TValue *)>;

    O5MSource * m_reader;
    TSubElementGetter m_func;

  public:
    class Iterator
    {
      O5MSource * m_reader;
      TValue m_val;
      TSubElementGetter m_func;

    public:
      Iterator() : m_reader(nullptr) {}
      explicit Iterator(O5MSource * reader, TSubElementGetter const & func) : m_reader(reader), m_func(func)
      {
        NextValue();
      }

      bool operator==(Iterator const & iter) const { return m_reader == iter.m_reader; }
      bool operator!=(Iterator const & iter) const { return !(*this == iter); }

      Iterator & operator++()
      {
        NextValue();
        return *this;
      }

      void NextValue() { m_reader = m_reader ? m_func(&m_val) : nullptr; }

      TValue const & operator*() const { return m_val; }
    };

    SubElements(O5MSource * reader, TSubElementGetter const & func) : m_reader(reader), m_func(func) {}

    void Skip()
    {
      while (m_reader && m_func(nullptr))
      { /* no-op */
      }
    }

    Iterator const begin() const { return Iterator(m_reader, m_func); }
    Iterator const end() const { return Iterator(); }
  };

  struct Entity
  {
    using EntityType = O5MSource::EntityType;
    EntityType type = EntityType::Reset;
    int64_t id = 0;
    uint64_t version = 0;
    double lon = 0;
    double lat = 0;
    int64_t timestamp = 0;
    int64_t changeset = 0;
    uint64_t uid = 0;
    char const * user = nullptr;

    using TRefs = SubElements<Member>;
    using TNodes = SubElements<int64_t>;
    using TTags = SubElements<KeyValue>;

    TRefs Members() const
    {
      return TRefs((type == EntityType::Relation) ? m_reader : nullptr,
                   [this](Member * val) { return (m_reader) ? m_reader->ReadMember(val) : nullptr; });
    }

    TNodes Nodes() const
    {
      return TNodes((type == EntityType::Way) ? m_reader : nullptr,
                    [this](int64_t * val) { return (m_reader) ? m_reader->ReadNd(val) : nullptr; });
    }

    TTags Tags() const
    {
      Members().Skip();
      Nodes().Skip();
      bool const validType = (type == EntityType::Node || type == EntityType::Way || type == EntityType::Relation);
      return TTags(validType ? m_reader : nullptr,
                   [this](KeyValue * val) { return (m_reader) ? m_reader->ReadStringPair(val) : nullptr; });
    }

    void SkipRemainder() const
    {
      if (!m_reader)
        return;

      if (!(type == EntityType::Node || type == EntityType::Way || type == EntityType::Relation))
        return;
      if (type == EntityType::Way)
        while (m_reader->ReadNd(nullptr))
          ;
      if (type == EntityType::Relation)
        while (m_reader->ReadMember(nullptr))
          ;

      while (m_reader->ReadStringPair(nullptr))
        ;
    }

    Entity() : m_reader(nullptr) {}
    Entity(O5MSource * reader) : m_reader(reader) {}

  private:
    O5MSource * const m_reader;
  };

  uint64_t ReadVarUInt()
  {
    uint8_t b;
    uint8_t i = 0;
    uint64_t ret = 0LL;

    do
    {
      b = m_buffer.Get();
      ret |= (uint64_t)(b & 0x7f) << (i++ * 7);
    }
    while (b & 0x80);
    size_t const rb = m_buffer.Recap();
    m_remainder -= rb;
    m_middlePartSize -= rb;
    return ret;
  }

  int64_t ReadVarInt()
  {
    uint64_t const ret = ReadVarUInt();
    return (ret & 1) ? (-int64_t(ret >> 1) - 1) : int64_t(ret >> 1);
  }

  O5MSource * ReadMember(Member * const ref)
  {
    if (m_middlePartSize <= 0)
      return nullptr;
    int64_t const delta = ReadVarInt();
    KeyValue kv;
    if (!ReadStringPair(&kv, true /*single string*/))
      return nullptr;

    int64_t current = 0;
    EntityType type = EntityType::Reset;

    switch (kv.key[0])
    {
    case '0':
    {
      current = (m_currentNodeRef += delta);
      type = EntityType::Node;
      break;
    }
    case '1':
    {
      current = (m_currentWayRef += delta);
      type = EntityType::Way;
      break;
    }
    case '2':
    {
      current = (m_currentRelationRef += delta);
      type = EntityType::Relation;
      break;
    }
    default: CHECK(false, ("Unexpected relation type:", kv.key));
    }
    if (ref)
    {
      ref->ref = current;
      ref->type = type;
      ref->role = &kv.key[1];
    }
    return this;
  }

  O5MSource * ReadNd(int64_t * const nd)
  {
    if (m_middlePartSize <= 0)
      return nullptr;
    m_currentNodeRef += ReadVarInt();
    if (nd)
      *nd = m_currentNodeRef;
    return this;
  }

  O5MSource * ReadStringPair(KeyValue * const kv, bool const single = false)
  {
    if (m_remainder == 0)
      return nullptr;

    uint64_t const key = ReadVarUInt();
    if (key)
    {
      // lookup table
      if (kv)
      {
        size_t const idx = (m_stringCurrentIndex + m_stringTable.size() - key) % m_stringTable.size();
        kv->key = m_stringTable[idx].key;
        kv->value = m_stringTable[idx].value;
      }
      return this;
    }

    char * pBuf = m_stringBuffer.data();
    size_t sizes[2] = {0, 0};
    for (size_t i = 0; i < (single ? 1 : 2); i++)
    {
      do
      {
        *pBuf = m_buffer.Get();
        sizes[i]++;
      }
      while (*(pBuf++));
    }
    size_t const rb = m_buffer.Recap();
    m_remainder -= rb;
    m_middlePartSize -= rb;

    if (sizes[0] + (single ? 0 : sizes[1]) <= StringTableRecord::MaxEntrySize)
    {
      memmove(m_stringTable[m_stringCurrentIndex].key, m_stringBuffer.data(), sizes[0]);
      memmove(m_stringTable[m_stringCurrentIndex].value, m_stringBuffer.data() + sizes[0], sizes[1]);
      if (kv)
        *kv = KeyValue(m_stringTable[m_stringCurrentIndex].key, m_stringTable[m_stringCurrentIndex].value);
      if (++m_stringCurrentIndex == m_stringTable.size())
        m_stringCurrentIndex = 0;
      return this;
    }

    if (kv)
      *kv = KeyValue(m_stringBuffer.data(), m_stringBuffer.data() + sizes[0]);
    return this;
  }

  void ReadIdAndVersion(Entity * const e)
  {
    e->id = (m_id += ReadVarInt());

    e->version = ReadVarUInt();
    if (e->version == 0)
    {
      e->timestamp = 0;
      e->changeset = 0;
      e->uid = 0;
      e->user = nullptr;
      return;
    }

    e->timestamp = (m_timestamp += ReadVarInt());
    if (m_timestamp)
    {
      e->changeset = (m_changeset += ReadVarInt());
      KeyValue kv;
      ReadStringPair(&kv);
      uint8_t i = 0;
      do
        e->uid |= (uint64_t)((*kv.key) & 0x7f) << (i++ * 7);
      while (*(kv.key++) & 0x80);
      e->user = kv.value;
    }
  }

#define DECODE_O5M_COORD(coord) (static_cast<double>(coord) / 1E+7)

  void ReadLonLat(Entity * const e)
  {
    e->lon = DECODE_O5M_COORD(m_lon += ReadVarInt());
    e->lat = DECODE_O5M_COORD(m_lat += ReadVarInt());
  }

  O5MSource * ReadEntity(Entity * const entity)
  {
    entity->SkipRemainder();

    entity->type = EntityType(m_buffer.Get());

    if (EntityType::End == entity->type)
      return nullptr;

    if (EntityType::Reset == entity->type)
    {
      Reset();
      return ReadEntity(entity);
    }

    entity->uid = 0;
    entity->user = nullptr;
    m_middlePartSize = 0;

    m_remainder = ReadVarUInt();  // entity size

    switch (entity->type)
    {
    case EntityType::Node:
    {
      ReadIdAndVersion(entity);
      ReadLonLat(entity);
    }
    break;
    case EntityType::Way:
    {
      ReadIdAndVersion(entity);
      m_middlePartSize = ReadVarUInt();
    }
    break;
    case EntityType::Relation:
    {
      ReadIdAndVersion(entity);
      m_middlePartSize = ReadVarUInt();
    }
    break;
    case EntityType::BBox:
    {
    }
    break;
    case EntityType::Timestamp:
    {
      ReadVarUInt();
    }
    break;
    case EntityType::Header:
    {
    }
    break;
    case EntityType::Sync:
    {
    }
    break;
    case EntityType::Jump:
    {
    }
    break;

    default: break;
    }
    return this;
  }

  void InitStringTable()
  {
    // When reading an .o5m coded file, we use a reference table which has 15,000 lines,
    // 250+2 characters each (for performance reasons: 256 characters).
    // Every string pair we encounter is copied into the table, with one exception: strings pairs
    // which are longer than 250 characters are interpreted but not copied into the table.

    m_stringCurrentIndex = 0;
    m_stringBuffer.resize(1024);
    m_stringTable.resize(15000);
  }

  void Reset()
  {
    m_currentNodeRef = 0;
    m_currentWayRef = 0;
    m_currentRelationRef = 0;
    m_id = 0;
    m_lon = 0;
    m_lat = 0;
    m_timestamp = 0;
    m_changeset = 0;
    m_remainder = 0;
  }

public:
  class Iterator
  {
    O5MSource * m_reader;
    Entity m_entity;

  public:
    Iterator() : m_reader(nullptr), m_entity(nullptr) {}
    Iterator(O5MSource * reader) : m_reader(reader), m_entity(reader)
    {
      m_reader->Reset();
      NextValue();
    }

    bool operator==(Iterator const & iter) const { return m_reader == iter.m_reader; }
    bool operator!=(Iterator const & iter) const { return !(*this == iter); }
    Iterator & operator++()
    {
      NextValue();
      return *this;
    }

    void NextValue() { m_reader = m_reader->ReadEntity(&m_entity) ? m_reader : nullptr; }

    Entity const & operator*() const { return m_entity; }
  };

  Iterator const begin() { return Iterator(this); }
  Iterator const end() { return Iterator(); }

  O5MSource(TReadFunc reader, size_t readBufferSizeInBytes = 60000) : m_buffer(reader, readBufferSizeInBytes)
  {
    if (EntityType::Reset != EntityType(m_buffer.Get()))
      throw std::runtime_error("Incorrect o5m start");
    CheckHeader();
    InitStringTable();
  }

  bool CheckHeader()
  {
    size_t const len = 4;
    if (EntityType::Header == EntityType(m_buffer.Get()) && ReadVarUInt() == len)
    {
      std::string sign(len, ' ');
      m_buffer.Read(reinterpret_cast<uint8_t *>(&sign[0]), len);
      if (sign == "o5m2" || sign == "o5c2")
        return true;
    }
    return false;
  }

  friend std::ostream & operator<<(std::ostream & s, O5MSource::Entity const & em)
  {
    s << EntityType(em.type) << " ID: " << em.id;
    if (em.version)
    {
      //      time_t timestamp = em.timestamp;
      //      tm stm = *gmtime(&timestamp);
      //      s << " Version: " << em.version << " timestamp: " << asctime_r(&stm, "%FT%TZ");

      s << " Version: " << em.version << " timestamp: " << em.timestamp;
      s << " changeset: " << em.changeset << " uid: " << em.uid << " user: " << em.user;
    }
    if (em.type == EntityType::Node)
      s << std::endl << " lon: " << em.lon << " lat: " << em.lat;
    return s;
  }
};

}  // namespace osm
