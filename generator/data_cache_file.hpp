#pragma once

#include "generator/osm_decl.hpp"

#include "coding/file_reader_stream.hpp"
#include "coding/file_writer_stream.hpp"

#include "base/logging.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/exception.hpp"


/// Classes for reading and writing any data in file with map of offsets for
/// fast searching in memory by some user-id.
namespace cache
{
  namespace detail
  {
    template <class TFile, class TValue> class file_map_t
    {
      typedef pair<uint64_t, TValue> element_t;
      typedef vector<element_t> id_cont_t;
      id_cont_t m_memory;
      TFile m_file;

      static const size_t s_max_count = 1024;

      struct element_less_t
      {
        bool operator() (element_t const & r1, element_t const & r2) const
        {
          return ((r1.first == r2.first) ? r1.second < r2.second : r1.first < r2.first);
        }
        bool operator() (element_t const & r1, uint64_t r2) const
        {
          return (r1.first < r2);
        }
        bool operator() (uint64_t r1, element_t const & r2) const
        {
          return (r1 < r2.first);
        }
      };

      size_t uint64_to_size(uint64_t v)
      {
        ASSERT ( v < numeric_limits<size_t>::max(), ("Value to long for memory address : ", v) );
        return static_cast<size_t>(v);
      }

    public:
      file_map_t(string const & name) : m_file(name.c_str()) {}

      string get_name() const { return m_file.GetName(); }

      void flush_to_file()
      {
        if (!m_memory.empty())
        {
          m_file.Write(&m_memory[0], m_memory.size() * sizeof(element_t));
          m_memory.clear();
        }
      }

      void read_to_memory()
      {
        m_memory.clear();
        uint64_t const fileSize = m_file.Size();
        if (fileSize == 0) return;

        LOG_SHORT(LINFO, ("Offsets reading is started for file ", get_name()));

        try
        {
          m_memory.resize(uint64_to_size(fileSize / sizeof(element_t)));
        }
        catch (exception const &) // bad_alloc
        {
          LOG(LCRITICAL, ("Insufficient memory for required offset map"));
        }

        m_file.Read(0, &m_memory[0], uint64_to_size(fileSize));

        sort(m_memory.begin(), m_memory.end(), element_less_t());

        LOG_SHORT(LINFO, ("Offsets reading is finished"));
      }

      void write(uint64_t k, TValue const & v)
      {
        if (m_memory.size() > s_max_count)
          flush_to_file();

        m_memory.push_back(make_pair(k, v));
      }

      bool read_one(uint64_t k, TValue & v) const
      {
        typename id_cont_t::const_iterator i =
            lower_bound(m_memory.begin(), m_memory.end(), k, element_less_t());
        if ((i != m_memory.end()) && ((*i).first == k))
        {
          v = (*i).second;
          return true;
        }
        return false;
      }

      typedef typename id_cont_t::const_iterator iter_t;
      pair<iter_t, iter_t> GetRange(uint64_t k) const
      {
        return equal_range(m_memory.begin(), m_memory.end(), k, element_less_t());
      }

      template <class ToDo> void for_each_ret(uint64_t k, ToDo & toDo) const
      {
        pair<iter_t, iter_t> range = GetRange(k);
        for (; range.first != range.second; ++range.first)
          if (toDo((*range.first).second))
            return;
      }
    };
  }

  template <class TStream, class TOffsetFile> class DataFileBase
  {
  public:
    typedef uint64_t user_id_t;

  protected:
    TStream m_stream;
    detail::file_map_t<TOffsetFile, uint64_t> m_offsets;

  public:
    DataFileBase(string const & name)
      : m_stream(name), m_offsets(name + OFFSET_EXT)
    {
    }
  };

  class DataFileWriter : public DataFileBase<FileWriterStream, FileWriter>
  {
    typedef DataFileBase<FileWriterStream, FileWriter> base_type;

    static const size_t s_max_count = 1024;

  public:
    DataFileWriter(string const & name) : base_type(name) {}

    template <class T> void Write(user_id_t id, T const & t)
    {
      m_offsets.write(id, m_stream.Pos());
      m_stream << t;
    }

    void SaveOffsets()
    {
      m_offsets.flush_to_file();
    }
  };

  class DataFileReader : public DataFileBase<FileReaderStream, FileReader>
  {
    typedef DataFileBase<FileReaderStream, FileReader> base_type;

  public:
    DataFileReader(string const & name) : base_type(name) {}

    template <class T> bool Read(user_id_t id, T & t)
    {
      uint64_t pos;
      if (m_offsets.read_one(id, pos))
      {
        m_stream.Seek(pos);
        m_stream >> t;
        return true;
      }
      else
      {
        LOG_SHORT(LWARNING, ("Can't find offset in file ", m_offsets.get_name(), " by id ", id) );
        return false;
      }
    }

    void LoadOffsets()
    {
      m_offsets.read_to_memory();
    }
  };


  template <class TNodesHolder, class TData, class TFile>
  class BaseFileHolder
  {
  protected:
    typedef typename TData::user_id_t user_id_t;

    TNodesHolder & m_nodes;

    TData m_ways, m_relations;

    typedef detail::file_map_t<TFile, uint64_t> offset_map_t;
    offset_map_t m_nodes2rel, m_ways2rel;

  public:
    BaseFileHolder(TNodesHolder & nodes, string const & dir)
      : m_nodes(nodes),
        m_ways(dir + WAYS_FILE),
        m_relations(dir + RELATIONS_FILE),
        m_nodes2rel(dir + NODES_FILE + ID2REL_EXT),
        m_ways2rel(dir + WAYS_FILE + ID2REL_EXT)
    {
    }
  };
}
