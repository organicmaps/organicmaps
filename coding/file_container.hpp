#pragma once
#include "file_reader.hpp"
#include "file_writer.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

class FilesContainerBase
{
protected:

  typedef string Tag;

  struct Info
  {
    Tag m_tag;
    uint64_t m_offset;
    uint64_t m_size;

    Info() {}
    Info(Tag const & tag, uint64_t offset) : m_tag(tag), m_offset(offset) {}
  };

  struct less_info
  {
    bool operator() (Info const & t1, Info const & t2) const
    {
      return (t1.m_tag < t2.m_tag);
    }
    bool operator() (Info const & t1, Tag const & t2) const
    {
      return (t1.m_tag < t2);
    }
    bool operator() (Tag const & t1, Info const & t2) const
    {
      return (t1 < t2.m_tag);
    }
  };

  typedef vector<Info> info_cont_t;
  info_cont_t m_info;

  void ReadInfo(FileReader & reader);
};

class FilesContainerR : public FilesContainerBase
{
  typedef FilesContainerBase base_type;

  FileReader m_source;

public:
  explicit FilesContainerR(string const & fName,
                           uint32_t logPageSize = 10,
                           uint32_t logPageCount = 10);

  FileReader GetReader(Tag const & tag) const;

  template <typename F> void ForEachTag(F f) const
  {
    for (size_t i = 0; i < m_info.size(); ++i)
      f(m_info[i].m_tag);
  }
};

class FilesContainerW : public FilesContainerBase
{
  typedef FilesContainerBase base_type;

  string m_name;

  uint64_t SaveCurrentSize();

  bool m_needRewrite;

public:
  FilesContainerW(string const & fName,
                  FileWriter::Op op = FileWriter::OP_WRITE_TRUNCATE);

  FileWriter GetWriter(Tag const & tag);

  void Append(string const & fName, Tag const & tag);
  void Append(vector<char> const & buffer, Tag const & tag);

  void Finish();
};
