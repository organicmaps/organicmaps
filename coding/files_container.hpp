#pragma once

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class FilesContainerBase
{
public:
  using Tag = std::string;

  struct TagInfo
  {
    TagInfo() = default;
    TagInfo(Tag const & tag, uint64_t offset) : m_tag(tag), m_offset(offset) {}

    Tag m_tag;
    uint64_t m_offset = 0;
    uint64_t m_size = 0;
  };

  /// Alignment of each new section that will be added to a file
  /// container, i.e. section's offset in bytes will be a multiple of
  /// this value.
  ///
  /// WARNING! Existing sections may not be properly aligned.
  static uint64_t const kSectionAlignment = 8;

  bool IsExist(Tag const & tag) const { return GetInfo(tag) != 0; }

  template <typename ToDo>
  void ForEachTagInfo(ToDo && toDo) const
  {
    std::for_each(m_info.begin(), m_info.end(), std::forward<ToDo>(toDo));
  }

protected:
  struct LessInfo
  {
    bool operator()(TagInfo const & t1, TagInfo const & t2) const { return (t1.m_tag < t2.m_tag); }
    bool operator()(TagInfo const & t1, Tag const & t2) const { return (t1.m_tag < t2); }
    bool operator()(Tag const & t1, TagInfo const & t2) const { return (t1 < t2.m_tag); }
  };

  struct LessOffset
  {
    bool operator()(TagInfo const & t1, TagInfo const & t2) const
    {
      if (t1.m_offset == t2.m_offset)
      {
        // Element with nonzero size should be the last one,
        // for correct append writer mode (FilesContainerW::GetWriter).
        return (t1.m_size < t2.m_size);
      }
      else
        return (t1.m_offset < t2.m_offset);
    }
    bool operator()(TagInfo const & t1, uint64_t const & t2) const { return (t1.m_offset < t2); }
    bool operator()(uint64_t const & t1, TagInfo const & t2) const { return (t1 < t2.m_offset); }
  };

  class EqualTag
  {
  public:
    EqualTag(Tag const & tag) : m_tag(tag) {}
    bool operator()(TagInfo const & t) const { return (t.m_tag == m_tag); }

  private:
    Tag const & m_tag;
  };

  TagInfo const * GetInfo(Tag const & tag) const;

  template <typename Reader>
  void ReadInfo(Reader & reader);

  using InfoContainer = std::vector<TagInfo>;
  InfoContainer m_info;
};

std::string DebugPrint(FilesContainerBase::TagInfo const & info);

class FilesContainerR : public FilesContainerBase
{
public:
  using TReader = ModelReaderPtr;

  explicit FilesContainerR(std::string const & filePath, uint32_t logPageSize = 10, uint32_t logPageCount = 10);
  explicit FilesContainerR(TReader const & file);

  TReader GetReader(Tag const & tag) const;

  template <typename F>
  void ForEachTag(F && f) const
  {
    for (size_t i = 0; i < m_info.size(); ++i)
      f(m_info[i].m_tag);
  }

  uint64_t GetFileSize() const { return m_source.Size(); }
  std::string const & GetFileName() const { return m_source.GetName(); }

  std::pair<uint64_t, uint64_t> GetAbsoluteOffsetAndSize(Tag const & tag) const;

private:
  TReader m_source;
};

namespace detail
{
class MappedFile
{
public:
  MappedFile() = default;
  ~MappedFile() { Close(); }

  void Open(std::string const & fName);
  void Close();

  class Handle
  {
  public:
    Handle() = default;

    Handle(char const * base, char const * alignBase, uint64_t size, uint64_t origSize)
      : m_base(base)
      , m_origBase(alignBase)
      , m_size(size)
      , m_origSize(origSize)
    {}

    Handle(Handle && h) { Assign(std::move(h)); }

    Handle & operator=(Handle && h)
    {
      Assign(std::move(h));
      return *this;
    }

    ~Handle();

    void Assign(Handle && h);

    void Unmap();

    bool IsValid() const { return (m_base != 0); }
    uint64_t GetSize() const { return m_size; }

    template <typename T>
    T const * GetData() const
    {
      ASSERT_EQUAL(m_size % sizeof(T), 0, ());
      return reinterpret_cast<T const *>(m_base);
    }

    template <typename T>
    size_t GetDataCount() const
    {
      ASSERT_EQUAL(m_size % sizeof(T), 0, ());
      return (m_size / sizeof(T));
    }

  private:
    void Reset();

    char const * m_base = nullptr;
    char const * m_origBase = nullptr;
    uint64_t m_size = 0;
    uint64_t m_origSize = 0;

    DISALLOW_COPY(Handle);
  };

  Handle Map(uint64_t offset, uint64_t size, std::string const & tag) const;

private:
#ifdef OMIM_OS_WINDOWS
  void * m_hFile = (void *)-1;
  void * m_hMapping = (void *)-1;
#else
  int m_fd = -1;
#endif

  DISALLOW_COPY(MappedFile);
};
}  // namespace detail

class FilesMappingContainer : public FilesContainerBase
{
public:
  using Handle = detail::MappedFile::Handle;

  /// Do nothing by default, call Open to attach to file.
  FilesMappingContainer() = default;
  explicit FilesMappingContainer(std::string const & fName);

  ~FilesMappingContainer();

  void Open(std::string const & fName);
  void Close();

  Handle Map(Tag const & tag) const;
  FileReader GetReader(Tag const & tag) const;

  std::string const & GetName() const { return m_name; }

private:
  std::string m_name;
  detail::MappedFile m_file;
};

class FilesContainerW : public FilesContainerBase
{
public:
  FilesContainerW(std::string const & fName, FileWriter::Op op = FileWriter::OP_WRITE_TRUNCATE);
  ~FilesContainerW();

  std::unique_ptr<FilesContainerWriter> GetWriter(Tag const & tag);

  void Write(std::string const & fPath, Tag const & tag);
  void Write(ModelReaderPtr reader, Tag const & tag);
  void Write(void const * buffer, size_t size, Tag const & tag);
  void Write(std::vector<char> const & buffer, Tag const & tag);
  void Write(std::vector<uint8_t> const & buffer, Tag const & tag);

  void Finish();

  /// Delete section with rewriting file.
  /// @precondition Container should be opened with FileWriter::OP_WRITE_EXISTING.
  void DeleteSection(Tag const & tag);

  std::string const & GetFileName() const { return m_name; }

private:
  uint64_t SaveCurrentSize();

  void Open(FileWriter::Op op);
  void StartNew();

  std::string m_name;
  bool m_needRewrite;
  bool m_finished;
};
