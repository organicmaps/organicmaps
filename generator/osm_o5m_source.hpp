//
//  osm_o5m_source.hpp
//  generator
//
//  Created by Sergey Yershov on 14.04.15.
//

#pragma once

#include "std/iostream.hpp"
#include "std/sstream.hpp"
#include "std/vector.hpp"
#include "std/iomanip.hpp"

namespace
{
  template <typename ReaderT>
  class StreamBuffer
  {
    using BufferT = std::vector<uint8_t>;

    ReaderT const & m_reader;
    BufferT m_buffer;
    size_t const m_buffer_base_size;
    size_t m_byte_counter;

    BufferT::value_type const * m_position;
    BufferT::value_type const * m_start;
    BufferT::value_type const * m_end;

  public:
    StreamBuffer(ReaderT const &reader, size_t buffer_size)
    : m_reader(reader)
    , m_buffer(buffer_size)
    , m_buffer_base_size(buffer_size)
    , m_byte_counter(0)
    {
      m_position = m_start = &m_buffer.front();
      m_end = (&m_buffer.back())+1;
      read_buffer();
    }

    size_t byte_counter()
    {
      size_t b = m_byte_counter;
      m_byte_counter = 0;
      return b;
    }

    inline BufferT::value_type get()
    {
      if(m_position == m_end)
        read_buffer();
      ++m_byte_counter;
      return *(m_position++);
    }

    inline BufferT::value_type peek()
    {
      if(m_position == m_end)
        read_buffer();
      return *m_position;
    }

    void skip(size_t size = 1)
    {
      if(m_position + size >= m_end)
      {
        size -= (m_end - m_position);
        for (read_buffer(); size > m_buffer.size(); size -= m_buffer.size())
          read_buffer();
      }
      m_position += size;
      m_byte_counter += size;
    }

    void read(BufferT::value_type * dest, size_t size) {
      if(m_position + size >= m_end)
      {
        size_t remainder = (m_end - m_position);
        memcpy(dest, m_position, remainder);
        size -= remainder;
        dest += remainder;
        for (read_buffer(); size > m_buffer.size(); size -= m_buffer.size())
        {
          memcpy(dest, m_buffer.data(), m_buffer.size());
          dest += m_buffer.size();
          read_buffer();
        }
      }
      memcpy(dest, m_position, size);
      m_position += size;
    }

  private:

    void read_buffer()
    {
      size_t readed_bytes = 0;

      if (m_buffer.size() != m_buffer_base_size)
        m_buffer.resize(m_buffer_base_size);

      readed_bytes = m_reader(m_buffer.data(), m_buffer.size());

      if (readed_bytes != m_buffer.size())
      {
        m_buffer.resize(readed_bytes);
        m_start = &m_buffer.front();
        m_end = (&m_buffer.back())+1;
      }
      m_position = m_start;
    }
  };
} // anonymouse namespace

namespace osm
{

  template <typename ReaderT>
  class O5MSource
  {
    template<typename TValue>
    using TReadFunc = std::function<O5MSource *(TValue*)>;

    struct StringTableRec
    {
      char ksize;
      char vsize;
      unsigned short int align;
      char k[254];
      char v[254];
    };

    struct KeyValue
    {
      char const * key = nullptr;
      char const * value = nullptr;

      KeyValue() = default;
      KeyValue(char const *k, char const *v) : key(k), value(v) {}
    };

    struct Member
    {
      int64_t ref = 0;
      uint8_t type = 0;
      char const * role = nullptr;
    };

    std::vector<StringTableRec> m_string_table;
    std::vector<char> m_string_buffer;
    size_t m_string_current_index;
    StreamBuffer<ReaderT> m_buffer;
    size_t m_remainder;
    int64_t m_delta_state[3];
    int64_t m_id = 0;
    int32_t m_lon = 0;
    int32_t m_lat = 0;
    uint64_t m_timestamp = 0;
    uint64_t m_changeset = 0;
    int64_t m_refarea_size = 0;

  public:
    enum ECommand
    {
      O5M_CMD_END=0xfe,
      O5M_CMD_NODE=0x10,
      O5M_CMD_WAY=0x11,
      O5M_CMD_REL=0x12,
      O5M_CMD_BBOX=0xdb,
      O5M_CMD_TSTAMP=0xdc,
      O5M_CMD_HEADER=0xe0,
      O5M_CMD_SYNC=0xee,
      O5M_CMD_JUMP=0xef,
      O5M_CMD_RESET=0xff
    };

    template <typename TValue, typename TFunc>
    class SubElements
    {
      O5MSource * m_reader;
      TFunc m_func;
    public:

      class iterator
      {
        O5MSource * m_reader;
        TValue m_val;
        TFunc m_func;
      public:


        iterator() : m_reader(nullptr) {}
        explicit iterator(O5MSource *reader, TFunc const & func)
        : m_reader(reader)
        , m_func(func)
        {
          next_val();
        }

        bool operator != (iterator const &iter) {return m_reader != iter.m_reader;}

        iterator & operator++() { next_val(); return *this; }
        iterator operator++(int) { next_val(); return *this; }

        void next_val() { m_reader = m_reader ? m_func(&m_val) : nullptr; }

        TValue const & operator*() const { return m_val; }
      };

      SubElements(O5MSource * reader, TFunc const & func) : m_reader(reader), m_func(func) {}

      void skip() { while(m_reader ? m_func(nullptr) : nullptr); }

      iterator const begin() const { return iterator(m_reader, m_func); }
      iterator const end() const { return iterator(); }
    };

    struct Entity
    {
      uint8_t type = 0;
      int64_t id = 0;
      uint64_t version = 0;
      int32_t lon = 0;
      int32_t lat = 0;
      int64_t timestamp = 0;
      int64_t changeset = 0;
      uint64_t uid = 0;
      char const * user = nullptr;

      using RefsT = SubElements<Member, TReadFunc<Member>>;
      using NodesT = SubElements<int64_t, TReadFunc<int64_t>>;
      using TagsT = SubElements<KeyValue, TReadFunc<KeyValue>>;

      RefsT members() const
      {
        return RefsT((type == O5M_CMD_REL) ? m_reader : nullptr, [this](Member *val)
                     {
                       return (m_reader) ? m_reader->ReadMember(val) : nullptr;
                     });
      }

      NodesT nodes() const
      {
        return NodesT((type == O5M_CMD_WAY) ? m_reader : nullptr, [this](int64_t *val)
                      {
                        return (m_reader) ? m_reader->ReadNd(val) : nullptr;
                      });
      }

      TagsT tags() const
      {
        members().skip();
        nodes().skip();
        bool const validType = (type == O5M_CMD_NODE || type == O5M_CMD_WAY || type == O5M_CMD_REL);
        return TagsT( validType ? m_reader : nullptr, [this](KeyValue *val)
                     {
                       return (m_reader) ? m_reader->ReadStringPair(val) : nullptr;
                     });
      }

      void SkipRemainder()
      {
        if (!(type == O5M_CMD_NODE || type == O5M_CMD_WAY || type == O5M_CMD_REL))
          return;
        if (type == O5M_CMD_WAY)
          while(m_reader ? m_reader->ReadNd(nullptr) : nullptr);
        if (type == O5M_CMD_REL)
          while(m_reader ? m_reader->ReadMember(nullptr) : nullptr);

        while(m_reader ? m_reader->ReadStringPair(nullptr) : nullptr);
      }

      Entity() : m_reader(nullptr) {}
      Entity(O5MSource * reader) : m_reader(reader) {}
    protected:
      O5MSource * m_reader;
    };

    uint64_t ReadUInt()
    {
      uint8_t b;
      uint8_t i = 0;
      uint64_t ret = 0LL;

      do
      {
        b = m_buffer.get();
        ret |= (uint64_t)(b & 0x7f) << (i++ * 7);
      } while ( b & 0x80 );
      size_t rb = m_buffer.byte_counter();
      m_remainder -= rb;
      m_refarea_size -= rb;
      return ret;
    }

    int64_t ReadInt()
    {
      uint64_t ret = ReadUInt();
      return (ret & 1) ? (-int64_t(ret >> 1) - 1) : int64_t(ret >> 1);
    }

    O5MSource * ReadMember(Member * ref)
    {
      if (m_refarea_size <= 0)
        return nullptr;
      int64_t delta = ReadInt();
      KeyValue kv;
      if(!ReadStringPair(&kv, true /*single string*/))
        return nullptr;
      m_delta_state[kv.key[0] - '0'] += delta;
      if (ref)
      {
        ref->ref = m_delta_state[kv.key[0] - '0'];
        ref->type = kv.key[0] - ' ';
        ref->role = &kv.key[1];
      }
      return this;
    }

    O5MSource * ReadNd(int64_t * nd)
    {
      if (m_refarea_size <= 0)
        return nullptr;
      (nd ? *nd : m_delta_state[0]) = (m_delta_state[0] += ReadInt());
      return this;
    }

    O5MSource * ReadStringPair(KeyValue * kv, bool single = false)
    {
      if (m_remainder == 0)
        return nullptr;

      uint64_t key = ReadUInt();
      if (key)
      {
        // lookup table
        if (kv)
          *kv = KeyValue(m_string_table[m_string_current_index-key].k, m_string_table[m_string_current_index-key].v);
        return this;
      }

      char *pBuf = m_string_buffer.data();
      size_t sizes[2] = {0,0};
      for (size_t i=0; i<(single?1:2); i++)
      {
        do
        {
          *pBuf = m_buffer.get();
          sizes[i]++;
        } while ( *(pBuf++) );
      }
      size_t rb = m_buffer.byte_counter();
      m_remainder -= rb;
      m_refarea_size -= rb;

      if (sizes[0]+sizes[1] < 500)
      {
        memcpy(m_string_table[m_string_current_index].k, m_string_buffer.data(), sizes[0]);
        memcpy(m_string_table[m_string_current_index].v, m_string_buffer.data()+sizes[0], sizes[1]);
        size_t key = m_string_current_index++;
        if (kv)
          *kv = KeyValue(m_string_table[key].k, m_string_table[key].v);
        return this;
      }

      if (kv)
        *kv = KeyValue(m_string_buffer.data(), m_string_buffer.data() + sizes[0]);
      return this;
    }

    void ReadIdAndVersion(Entity * e)
    {
      e->id = (m_id += ReadInt());

      if ((e->version = ReadUInt()) == 0)
      {
        e->timestamp = 0;
        e->changeset = 0;
        e->uid = 0;
        e->user = nullptr;
        return;
      }

      e->timestamp = (m_timestamp += ReadInt());
      if (m_timestamp)
      {
        e->changeset = (m_changeset += ReadInt());
        KeyValue kv;
        ReadStringPair(&kv);
        uint8_t i = 0;
        do e->uid |= (uint64_t)((*kv.key) & 0x7f) << (i++ * 7);
        while ( *(kv.key++) & 0x80 );
        e->user = kv.value;
      }
    }

    void ReadLonLat(Entity * e)
    {
      e->lon = (m_lon += ReadInt());
      e->lat = (m_lat += ReadInt());
    }


    O5MSource * ReadEntity(Entity * entity)
    {

      entity->SkipRemainder();

      entity->type = m_buffer.get();

      if (O5M_CMD_END == entity->type)
        return nullptr;

      if (O5M_CMD_RESET == entity->type)
      {
        Reset();
        return ReadEntity(entity);
      }

      entity->uid = 0;
      entity->user = nullptr;
      m_refarea_size = 0;

      m_remainder = ReadUInt(); /* entity size */

      switch (entity->type) {
        case O5M_CMD_NODE: {
          ReadIdAndVersion(entity);
          ReadLonLat(entity);
        } break;
        case O5M_CMD_WAY: {
          ReadIdAndVersion(entity);
          m_refarea_size = ReadUInt();
        } break;
        case O5M_CMD_REL: {
          ReadIdAndVersion(entity);
          m_refarea_size = ReadUInt();
        } break;
        case O5M_CMD_BBOX: {
        } break;
        case O5M_CMD_TSTAMP: {
          ReadUInt();
        } break;
        case O5M_CMD_HEADER: {
        } break;
        case O5M_CMD_SYNC: {
        } break;
        case O5M_CMD_JUMP: {
        } break;

        default:
          break;
      }
      return this;
    }

    void InitStringTable()
    {
      m_string_current_index = 0;
      m_string_buffer.resize(1024);
      m_string_table.resize(15000);
    }

    void Reset()
    {
      m_delta_state[0] = 0;
      m_delta_state[1] = 0;
      m_delta_state[2] = 0;
      m_id = 0;
      m_lon = 0;
      m_lat = 0;
      m_timestamp = 0;
      m_changeset = 0;
      m_remainder = 0;
    }

  public:

    static char const * cmd_to_text(uint8_t cmd)
    {
      switch (cmd) {
        case O5M_CMD_END: return "O5M_CMD_END"; break;
        case O5M_CMD_NODE: return "O5M_CMD_NODE"; break;
        case O5M_CMD_WAY: return "O5M_CMD_WAY"; break;
        case O5M_CMD_REL: return "O5M_CMD_REL"; break;
        case O5M_CMD_BBOX: return "O5M_CMD_BBOX"; break;
        case O5M_CMD_TSTAMP: return "O5M_CMD_TSTAMP"; break;
        case O5M_CMD_HEADER: return "O5M_CMD_HEADER"; break;
        case O5M_CMD_SYNC: return "O5M_CMD_SYNC"; break;
        case O5M_CMD_JUMP: return "O5M_CMD_JUMP"; break;
        case O5M_CMD_RESET: return "O5M_CMD_RESET"; break;
        default: return "Unknown command";
      }
    }

    class iterator
    {
      O5MSource * m_reader;
      Entity m_entity;

    public:
      iterator() : m_reader(nullptr), m_entity(nullptr) {}
      iterator(O5MSource *reader) : m_reader(reader), m_entity(reader)
      {
        m_reader->Reset();
        next_val();
      }

      bool operator != (iterator const &iter) {return m_reader != iter.m_reader;}
      iterator & operator++() { next_val(); return *this; }
      iterator operator++(int) { next_val(); return *this; }

      void next_val() { m_reader = m_reader->ReadEntity(&m_entity) ? m_reader : nullptr; }

      Entity const & operator*() const { return m_entity; }
    };

    iterator const begin() {return iterator(this);}
    iterator const end() {return iterator();}

    O5MSource(ReaderT const & reader) : m_buffer(reader, 10000 /* buffer size */)
    {
      if (m_buffer.get() != O5M_CMD_RESET)
      {
        throw std::runtime_error("Incorrect o5m start");
      }
      CheckHeader();
      InitStringTable();
    }

    bool CheckHeader()
    {
      size_t len = 4;
      if (m_buffer.get() == O5M_CMD_HEADER && ReadUInt() == len)
      {
        std::string sign(len,' ');
        m_buffer.read((uint8_t *)sign.data(), len);
        if (sign == "o5m2" || sign == "o5c2")
          return true;
      }
      return false;
    }

    friend std::ostream & operator << (std::ostream & s, O5MSource::Entity const & em)
    {
      s << cmd_to_text(em.type) << " ID: " << em.id;
      if (em.version)
      {
        std::time_t timestamp = em.timestamp;
        std::tm stm = *std::gmtime(&timestamp);
        s << " Version: " << em.version << " timestamp: " << std::put_time(&stm, "%FT%TZ");
        s << " changeset: " << em.changeset << " uid: " << em.uid << " user: " << em.user;
      }
      if (em.type == O5M_CMD_NODE)
      {
        s << std::endl << " lon: " << em.lon << " lat: " << em.lat;
      }
      return s;
    }
  };

  using TReadFunc = std::function<size_t(uint8_t *,size_t)>;
  typedef O5MSource<TReadFunc> O5MSourceReader;

} // namespace osm
