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

  struct LessInfo
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
  struct LessOffset
  {
    bool operator() (Info const & t1, Info const & t2) const
    {
      if (t1.m_offset == t2.m_offset)
      {
        // Element with nonzero size should be the last one,
        // for correct append writer mode (FilesContainerW::GetWriter).
        ASSERT ( t1.m_size == 0 || t2.m_size == 0, (t1.m_size, t2.m_size) );
        return (t1.m_size < t2.m_size);
      }
      else
        return (t1.m_offset < t2.m_offset);
    }
    bool operator() (Info const & t1, uint64_t const & t2) const
    {
      return (t1.m_offset < t2);
    }
    bool operator() (uint64_t const & t1, Info const & t2) const
    {
      return (t1 < t2.m_offset);
    }
  };
  class EqualTag
  {
    Tag const & m_tag;
  public:
    EqualTag(Tag const & tag) : m_tag(tag) {}
    bool operator() (Info const & t) const
    {
      return (t.m_tag == m_tag);
    }
  };


  typedef vector<Info> InfoContainer;
  InfoContainer m_info;

  template <class ReaderT>
  void ReadInfo(ReaderT & reader);
};

class FilesContainerR : public FilesContainerBase
{
public:
  typedef ModelReaderPtr ReaderT;

  explicit FilesContainerR(string const & fName,
                           uint32_t logPageSize = 10,
                           uint32_t logPageCount = 10);
  FilesContainerR(ReaderT const & file);

  ReaderT GetReader(Tag const & tag) const;

  bool IsReaderExist(Tag const & tag) const;

  template <typename F> void ForEachTag(F f) const
  {
    for (size_t i = 0; i < m_info.size(); ++i)
      f(m_info[i].m_tag);
  }

private:
  ReaderT m_source;
};

class FilesContainerW : public FilesContainerBase
{
public:
  FilesContainerW(string const & fName,
                  FileWriter::Op op = FileWriter::OP_WRITE_TRUNCATE);
  ~FilesContainerW();

  FileWriter GetWriter(Tag const & tag);

  /// @todo Subclass from FileWriter to check write bounds (avoid overrun).
  /// Return this object with additional check.
  /// @precondition Container should be constructed with OP_WRITE_EXISTING.
  FileWriter GetExistingWriter(Tag const & tag);

  /// @name Append to existing container.
  /// @precondition Container should be constructed with OP_APPEND.
  //@{
  void Append(string const & fPath, Tag const & tag);
  void Append(ModelReaderPtr reader, Tag const & tag);
  void Append(vector<char> const & buffer, Tag const & tag);
  //@}

  void Finish();

private:
  uint64_t SaveCurrentSize();

  string m_name;
  bool m_bNeedRewrite;
  bool m_bFinished;
};
