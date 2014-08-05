#include "../base/SRC_FIRST.hpp"

#include "file_container.hpp"
#include "read_write_utils.hpp"
#include "write_to_sink.hpp"
#include "internal/file_data.hpp"


template <class TSource> void Read(TSource & src, FilesContainerBase::Info & i)
{
  rw::Read(src, i.m_tag);

  i.m_offset = ReadVarUint<uint64_t>(src);
  i.m_size = ReadVarUint<uint64_t>(src);
}

template <class TSink> void Write(TSink & sink, FilesContainerBase::Info const & i)
{
  rw::Write(sink, i.m_tag);

  WriteVarUint(sink, i.m_offset);
  WriteVarUint(sink, i.m_size);
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerBase
/////////////////////////////////////////////////////////////////////////////

template <class ReaderT>
void FilesContainerBase::ReadInfo(ReaderT & reader)
{
  uint64_t offset = ReadPrimitiveFromPos<uint64_t>(reader, 0);

  ReaderSource<ReaderT> src(reader);
  src.Skip(offset);

  rw::Read(src, m_info);
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerR
/////////////////////////////////////////////////////////////////////////////

FilesContainerR::FilesContainerR(string const & fName,
                                 uint32_t logPageSize,
                                 uint32_t logPageCount)
  : m_source(new FileReader(fName, logPageSize, logPageCount))
{
  ReadInfo(m_source);
}

FilesContainerR::FilesContainerR(ReaderT const & file)
  : m_source(file)
{
  ReadInfo(m_source);
}

FilesContainerR::ReaderT FilesContainerR::GetReader(Tag const & tag) const
{
  InfoContainer::const_iterator i =
    lower_bound(m_info.begin(), m_info.end(), tag, LessInfo());

  if (i != m_info.end() && i->m_tag == tag)
    return m_source.SubReader(i->m_offset, i->m_size);
  else
    MYTHROW(Reader::OpenException, (tag));
}

bool FilesContainerR::IsReaderExist(Tag const & tag) const
{
  InfoContainer::const_iterator i =
    lower_bound(m_info.begin(), m_info.end(), tag, LessInfo());

  return (i != m_info.end() && i->m_tag == tag);
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerW
/////////////////////////////////////////////////////////////////////////////

FilesContainerW::FilesContainerW(string const & fName, FileWriter::Op op)
: m_name(fName), m_bFinished(false)
{
  Open(op);
}

void FilesContainerW::Open(FileWriter::Op op)
{
  m_bNeedRewrite = true;

  switch (op)
  {
  case FileWriter::OP_WRITE_TRUNCATE:
    break;

  case FileWriter::OP_WRITE_EXISTING:
    {
      {
        // read an existing service info
        FileReader reader(m_name);
        ReadInfo(reader);
      }

      // Important: in append mode we should sort info-vector by offsets
      sort(m_info.begin(), m_info.end(), LessOffset());
      break;
    }

  default:
    ASSERT ( false, ("Unsupported options") );
    break;
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
  m_bNeedRewrite = false;
}

FilesContainerW::~FilesContainerW()
{
  if (!m_bFinished)
    Finish();
}

uint64_t FilesContainerW::SaveCurrentSize()
{
  ASSERT(!m_bFinished, ());
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
    {
      if (m_info[i].m_tag != tag)
        contW.Write(contR.GetReader(m_info[i].m_tag), m_info[i].m_tag);
    }
  }

  // swap files
  if (!my::DeleteFileX(m_name) || !my::RenameFileX(m_name + ".tmp", m_name))
    MYTHROW(RootException, ("Can't rename file", m_name, "Sharing violation or disk error!"));

  // do open to update m_info
  Open(FileWriter::OP_WRITE_EXISTING);
}

FileWriter FilesContainerW::GetWriter(Tag const & tag)
{
  ASSERT(!m_bFinished, ());

  InfoContainer::const_iterator it = find_if(m_info.begin(), m_info.end(), EqualTag(tag));
  if (it != m_info.end())
  {
    if (it+1 == m_info.end())
    {
      m_info.pop_back();

      if (m_info.empty())
        StartNew();
      else
        m_bNeedRewrite = true;
    }
    else
    {
      DeleteSection(it->m_tag);
    }
  }

  if (m_bNeedRewrite)
  {
    m_bNeedRewrite = false;
    ASSERT ( !m_info.empty(), () );

    uint64_t const curr = m_info.back().m_offset + m_info.back().m_size;
    m_info.push_back(Info(tag, curr));

    FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING, true);
    writer.Seek(curr);
    return writer;
  }
  else
  {
    uint64_t const curr = SaveCurrentSize();
    m_info.push_back(Info(tag, curr));
    return FileWriter(m_name, FileWriter::OP_APPEND);
  }
}

void FilesContainerW::Write(string const & fPath, Tag const & tag)
{
  Write(new FileReader(fPath), tag);
}

void FilesContainerW::Write(ModelReaderPtr reader, Tag const & tag)
{
  ReaderSource<ModelReaderPtr> src(reader);
  FileWriter writer = GetWriter(tag);

  rw::ReadAndWrite(src, writer, 4*1024);
}

void FilesContainerW::Write(vector<char> const & buffer, Tag const & tag)
{
  if (!buffer.empty())
    GetWriter(tag).Write(&buffer[0], buffer.size());
}

void FilesContainerW::Finish()
{
  ASSERT(!m_bFinished, ());

  uint64_t const curr = SaveCurrentSize();

  FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING);
  writer.Seek(0);
  WriteToSink(writer, curr);
  writer.Seek(curr);

  sort(m_info.begin(), m_info.end(), LessInfo());

  rw::Write(writer, m_info);

  m_bFinished = true;
}
