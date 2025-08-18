#include "coding/files_container.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include <cstring>
#include <sstream>

#ifdef OMIM_OS_WINDOWS
#include "std/windows.hpp"
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>  // _SC_PAGESIZE
#endif

#include <errno.h>

template <typename Source, typename Info>
void Read(Source & src, Info & i)
{
  rw::Read(src, i.m_tag);

  i.m_offset = ReadVarUint<uint64_t>(src);
  i.m_size = ReadVarUint<uint64_t>(src);
}

template <typename Sink, typename Info>
void Write(Sink & sink, Info const & i)
{
  rw::Write(sink, i.m_tag);

  WriteVarUint(sink, i.m_offset);
  WriteVarUint(sink, i.m_size);
}

std::string DebugPrint(FilesContainerBase::TagInfo const & info)
{
  std::ostringstream ss;
  ss << "{ " << info.m_tag << ", " << info.m_offset << ", " << info.m_size << " }";
  return ss.str();
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerBase
/////////////////////////////////////////////////////////////////////////////

template <typename Reader>
void FilesContainerBase::ReadInfo(Reader & reader)
{
  uint64_t offset = ReadPrimitiveFromPos<uint64_t>(reader, 0);

  ReaderSource<Reader> src(reader);
  src.Skip(offset);

  rw::Read(src, m_info);
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerR
/////////////////////////////////////////////////////////////////////////////

FilesContainerR::FilesContainerR(std::string const & filePath, uint32_t logPageSize, uint32_t logPageCount)
  : m_source(std::make_unique<FileReader>(filePath, logPageSize, logPageCount))
{
  ReadInfo(m_source);
}

FilesContainerR::FilesContainerR(TReader const & file) : m_source(file)
{
  ReadInfo(m_source);
}

FilesContainerR::TReader FilesContainerR::GetReader(Tag const & tag) const
{
  TagInfo const * p = GetInfo(tag);
  if (!p)
    MYTHROW(Reader::OpenException, ("Can't find section:", GetFileName(), tag));
  return m_source.SubReader(p->m_offset, p->m_size);
}

std::pair<uint64_t, uint64_t> FilesContainerR::GetAbsoluteOffsetAndSize(Tag const & tag) const
{
  TagInfo const * p = GetInfo(tag);
  if (!p)
    MYTHROW(Reader::OpenException, ("Can't find section:", GetFileName(), tag));

  auto reader = dynamic_cast<FileReader const *>(m_source.GetPtr());
  uint64_t const offset = reader ? reader->GetOffset() : 0;
  return std::make_pair(offset + p->m_offset, p->m_size);
}

FilesContainerBase::TagInfo const * FilesContainerBase::GetInfo(Tag const & tag) const
{
  auto i = lower_bound(m_info.begin(), m_info.end(), tag, LessInfo());
  if (i != m_info.end() && i->m_tag == tag)
    return &(*i);
  else
    return 0;
}

namespace detail
{
/////////////////////////////////////////////////////////////////////////////
// MappedFile
/////////////////////////////////////////////////////////////////////////////
void MappedFile::Open(std::string const & fName)
{
  Close();

#ifdef OMIM_OS_WINDOWS
  m_hFile = CreateFileA(fName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    MYTHROW(Reader::OpenException, ("Can't open file:", fName, "win last error:", GetLastError()));
  m_hMapping = CreateFileMappingA(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m_hMapping == NULL)
    MYTHROW(Reader::OpenException, ("Can't create file's Windows mapping:", fName, "win last error:", GetLastError()));
#else
  m_fd = open(fName.c_str(), O_RDONLY | O_NONBLOCK);
  if (m_fd == -1)
  {
    if (errno == EMFILE || errno == ENFILE)
      MYTHROW(Reader::TooManyFilesException, ("Can't open file:", fName, ", reason:", strerror(errno)));
    else
      MYTHROW(Reader::OpenException, ("Can't open file:", fName, ", reason:", strerror(errno)));
  }
#endif
}

void MappedFile::Close()
{
#ifdef OMIM_OS_WINDOWS
  if (m_hMapping != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hMapping);
    m_hMapping = INVALID_HANDLE_VALUE;
  }
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
  }
#else
  if (m_fd != -1)
  {
    close(m_fd);
    m_fd = -1;
  }
#endif
}

MappedFile::Handle MappedFile::Map(uint64_t offset, uint64_t size, std::string const & tag) const
{
#ifdef OMIM_OS_WINDOWS
  SYSTEM_INFO sysInfo;
  memset(&sysInfo, 0, sizeof(sysInfo));
  GetSystemInfo(&sysInfo);
  long const align = sysInfo.dwAllocationGranularity;
#else
  long const align = sysconf(_SC_PAGESIZE);
#endif

  uint64_t const alignedOffset = (offset / align) * align;
  ASSERT_LESS_OR_EQUAL(alignedOffset, offset, ());
  uint64_t const length = size + (offset - alignedOffset);
  ASSERT_GREATER_OR_EQUAL(length, size, ());

#ifdef OMIM_OS_WINDOWS
  void * pMap =
      MapViewOfFile(m_hMapping, FILE_MAP_READ, alignedOffset >> (sizeof(DWORD) * 8), DWORD(alignedOffset), length);
  if (pMap == NULL)
    MYTHROW(Reader::OpenException,
            ("Can't map section:", tag, "with [offset, size]:", offset, size, "win last error:", GetLastError()));
#else
  void * pMap = mmap(0, static_cast<size_t>(length), PROT_READ, MAP_SHARED, m_fd, static_cast<off_t>(alignedOffset));
  if (pMap == MAP_FAILED)
    MYTHROW(Reader::OpenException,
            ("Can't map section:", tag, "with [offset, size]:", offset, size, "errno:", strerror(errno)));
#endif

  char const * data = reinterpret_cast<char const *>(pMap);
  char const * d = data + (offset - alignedOffset);
  return Handle(d, data, size, length);
}

}  // namespace detail

/////////////////////////////////////////////////////////////////////////////
// FilesMappingContainer
/////////////////////////////////////////////////////////////////////////////

FilesMappingContainer::FilesMappingContainer(std::string const & fName)
{
  Open(fName);
}

FilesMappingContainer::~FilesMappingContainer()
{
  Close();
}

void FilesMappingContainer::Open(std::string const & fName)
{
  {
    FileReader reader(fName);
    ReadInfo(reader);
  }

  m_file.Open(fName);

  m_name = fName;
}

void FilesMappingContainer::Close()
{
  m_file.Close();

  m_name.clear();
}

FilesMappingContainer::Handle FilesMappingContainer::Map(Tag const & tag) const
{
  TagInfo const * p = GetInfo(tag);
  if (!p)
    MYTHROW(Reader::OpenException, ("Can't find section:", m_name, tag));

  ASSERT_EQUAL(tag, p->m_tag, ());
  return m_file.Map(p->m_offset, p->m_size, tag);
}

FileReader FilesMappingContainer::GetReader(Tag const & tag) const
{
  TagInfo const * p = GetInfo(tag);
  if (!p)
    MYTHROW(Reader::OpenException, ("Can't find section:", m_name, tag));
  return FileReader(m_name).SubReader(p->m_offset, p->m_size);
}

/////////////////////////////////////////////////////////////////////////////
// FilesMappingContainer::Handle
/////////////////////////////////////////////////////////////////////////////

detail::MappedFile::Handle::~Handle()
{
  Unmap();
}

void FilesMappingContainer::Handle::Assign(Handle && h)
{
  Unmap();

  m_base = h.m_base;
  m_origBase = h.m_origBase;
  m_size = h.m_size;
  m_origSize = h.m_origSize;

  h.Reset();
}

void FilesMappingContainer::Handle::Unmap()
{
  if (IsValid())
  {
#ifdef OMIM_OS_WINDOWS
    VERIFY(UnmapViewOfFile(m_origBase), ());
#else
    VERIFY(0 == munmap((void *)m_origBase, static_cast<size_t>(m_origSize)), ());
#endif
    Reset();
  }
}

void FilesMappingContainer::Handle::Reset()
{
  m_base = m_origBase = 0;
  m_size = m_origSize = 0;
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerW
/////////////////////////////////////////////////////////////////////////////

FilesContainerW::FilesContainerW(std::string const & fName, FileWriter::Op op) : m_name(fName), m_finished(false)
{
  Open(op);
}

void FilesContainerW::Open(FileWriter::Op op)
{
  m_needRewrite = true;

  switch (op)
  {
  case FileWriter::OP_WRITE_TRUNCATE: break;

  case FileWriter::OP_WRITE_EXISTING:
  {
    // read an existing service info
    FileReader reader(m_name);
    ReadInfo(reader);
  }

    // Important: in append mode we should sort info-vector by offsets
    sort(m_info.begin(), m_info.end(), LessOffset());

    // Check that all offsets are unique
#ifdef DEBUG
    for (size_t i = 1; i < m_info.size(); ++i)
      ASSERT(m_info[i - 1].m_offset < m_info[i].m_offset || m_info[i - 1].m_size == 0 || m_info[i].m_size == 0, ());
#endif
    break;

  default: ASSERT(false, ("Unsupported options")); break;
  }

  if (m_info.empty())
    StartNew();
}

void FilesContainerW::StartNew()
{
  // leave space for offset to service info
  FileWriter writer(m_name);
  uint64_t skip = 0;
  writer.Write(&skip, sizeof(skip));
  m_needRewrite = false;
}

FilesContainerW::~FilesContainerW()
{
  if (!m_finished)
    Finish();
}

uint64_t FilesContainerW::SaveCurrentSize()
{
  ASSERT(!m_finished, ());
  uint64_t const curr = FileReader(m_name).Size();
  if (!m_info.empty())
    m_info.back().m_size = curr - m_info.back().m_offset;
  return curr;
}

void FilesContainerW::DeleteSection(Tag const & tag)
{
  {
    // rewrite files on disk
    FilesContainerR contR(m_name);
    FilesContainerW contW(m_name + ".tmp");

    for (size_t i = 0; i < m_info.size(); ++i)
      if (m_info[i].m_tag != tag)
        contW.Write(contR.GetReader(m_info[i].m_tag), m_info[i].m_tag);
  }

  // swap files
  if (!base::DeleteFileX(m_name) || !base::RenameFileX(m_name + ".tmp", m_name))
    MYTHROW(RootException, ("Can't rename file", m_name, "Sharing violation or disk error!"));

  // do open to update m_info
  Open(FileWriter::OP_WRITE_EXISTING);
}

std::unique_ptr<FilesContainerWriter> FilesContainerW::GetWriter(Tag const & tag)
{
  ASSERT(!m_finished, ());

  InfoContainer::const_iterator it = find_if(m_info.begin(), m_info.end(), EqualTag(tag));
  if (it != m_info.end())
  {
    if (it + 1 == m_info.end())
    {
      m_info.pop_back();

      if (m_info.empty())
        StartNew();
      else
        m_needRewrite = true;
    }
    else
    {
      DeleteSection(it->m_tag);
    }
  }

  if (m_needRewrite)
  {
    m_needRewrite = false;

    ASSERT(!m_info.empty(), ());

    uint64_t const curr = m_info.back().m_offset + m_info.back().m_size;
    auto writer = std::make_unique<TruncatingFileWriter>(m_name);
    writer->Seek(curr);
    writer->WritePaddingByPos(kSectionAlignment);

    m_info.emplace_back(tag, writer->Pos());
    ASSERT_EQUAL(m_info.back().m_offset % kSectionAlignment, 0, ());
    return writer;
  }
  else
  {
    SaveCurrentSize();

    auto writer = std::make_unique<FilesContainerWriter>(m_name, FileWriter::OP_APPEND);
    writer->WritePaddingByPos(kSectionAlignment);

    m_info.emplace_back(tag, writer->Pos());
    ASSERT_EQUAL(m_info.back().m_offset % kSectionAlignment, 0, ());
    return writer;
  }
}

void FilesContainerW::Write(std::string const & fPath, Tag const & tag)
{
  Write(ModelReaderPtr(std::make_unique<FileReader>(fPath)), tag);
}

void FilesContainerW::Write(ModelReaderPtr reader, Tag const & tag)
{
  ReaderSource<ModelReaderPtr> src(reader);
  auto writer = GetWriter(tag);

  rw::ReadAndWrite(src, *writer);
}

void FilesContainerW::Write(void const * buffer, size_t size, Tag const & tag)
{
  if (size != 0)
    GetWriter(tag)->Write(buffer, size);
}

void FilesContainerW::Write(std::vector<char> const & buffer, Tag const & tag)
{
  Write(buffer.data(), buffer.size(), tag);
}

void FilesContainerW::Write(std::vector<uint8_t> const & buffer, Tag const & tag)
{
  Write(buffer.data(), buffer.size(), tag);
}

void FilesContainerW::Finish()
{
  ASSERT(!m_finished, ());

  uint64_t const curr = SaveCurrentSize();

  FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING);
  writer.Seek(0);
  WriteToSink(writer, curr);
  writer.Seek(curr);

  sort(m_info.begin(), m_info.end(), LessInfo());

  rw::Write(writer, m_info);

  m_finished = true;
}
