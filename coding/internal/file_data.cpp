#include "coding/internal/file_data.hpp"

#include "reader.hpp" // For Reader exceptions.
#include "writer.hpp" // For Writer exceptions.
#include "coding/constants.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "std/cerrno.hpp"
#include "std/cstring.hpp"
#include "std/exception.hpp"
#include "std/fstream.hpp"
#include "std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS
  #include <io.h>
#endif

#ifdef OMIM_OS_TIZEN
#include "tizen/inc/FIo.hpp"
#endif


namespace my
{

FileData::FileData(string const & fileName, Op op)
    : m_FileName(fileName), m_Op(op)
{
  char const * const modes [] = {"rb", "wb", "r+b", "ab"};
#ifdef OMIM_OS_TIZEN
  m_File = new Tizen::Io::File();
  result const error = m_File->Construct(fileName.c_str(), modes[op]);
  if (error == E_SUCCESS)
  {
    return;
  }
#else
  m_File = fopen(fileName.c_str(), modes[op]);
  if (m_File)
    return;

  if (op == OP_WRITE_EXISTING)
  {
    // Special case, since "r+b" fails if file doesn't exist.
    m_File = fopen(fileName.c_str(), "wb");
    if (m_File)
      return;
  }
#endif

  // if we're here - something bad is happened
  if (m_Op != OP_READ)
    MYTHROW(Writer::OpenException, (GetErrorProlog()));
  else
    MYTHROW(Reader::OpenException, (GetErrorProlog()));
}

FileData::~FileData()
{
#ifdef OMIM_OS_TIZEN
  delete m_File;
#else
  if (m_File)
  {
    if (fclose(m_File))
      LOG(LWARNING, ("Error closing file", GetErrorProlog()));
  }
#endif
}

string FileData::GetErrorProlog() const
{
  char const * s;
  switch (m_Op)
  {
  case OP_READ: s = "Read"; break;
  case OP_WRITE_TRUNCATE: s = "Write truncate"; break;
  case OP_WRITE_EXISTING: s = "Write existing"; break;
  case OP_APPEND: s = "Append"; break;
  }

  return m_FileName + "; " + s + "; " + strerror(errno);
}

static int64_t const INVALID_POS = -1;

uint64_t FileData::Size() const
{
#ifdef OMIM_OS_TIZEN
  Tizen::Io::FileAttributes attr;
  result const error = Tizen::Io::File::GetAttributes(m_FileName.c_str(), attr);
  if (IsFailed(error))
    MYTHROW(Reader::SizeException, (m_FileName, m_Op, error));
  return attr.GetFileSize();
#else
  int64_t const pos = ftell64(m_File);
  if (pos == INVALID_POS)
    MYTHROW(Reader::SizeException, (GetErrorProlog(), pos));

  if (fseek64(m_File, 0, SEEK_END))
    MYTHROW(Reader::SizeException, (GetErrorProlog()));

  int64_t const size = ftell64(m_File);
  if (size == INVALID_POS)
    MYTHROW(Reader::SizeException, (GetErrorProlog(), size));

  if (fseek64(m_File, pos, SEEK_SET))
    MYTHROW(Reader::SizeException, (GetErrorProlog(), pos));

  ASSERT_GREATER_OR_EQUAL(size, 0, ());
  return static_cast<uint64_t>(size);
#endif
}

void FileData::Read(uint64_t pos, void * p, size_t size)
{
#ifdef OMIM_OS_TIZEN
  result error = m_File->Seek(Tizen::Io::FILESEEKPOSITION_BEGIN, pos);
  if (IsFailed(error))
    MYTHROW(Reader::ReadException, (error, pos));
  int const bytesRead = m_File->Read(p, size);
  error = GetLastResult();
  if (static_cast<size_t>(bytesRead) != size || IsFailed(error))
    MYTHROW(Reader::ReadException, (m_FileName, m_Op, error, bytesRead, pos, size));
#else
  if (fseek64(m_File, pos, SEEK_SET))
    MYTHROW(Reader::ReadException, (GetErrorProlog(), pos));

  size_t const bytesRead = fread(p, 1, size, m_File);
  if (bytesRead != size || ferror(m_File))
    MYTHROW(Reader::ReadException, (GetErrorProlog(), bytesRead, pos, size));
#endif
}

uint64_t FileData::Pos() const
{
#ifdef OMIM_OS_TIZEN
  int const pos = m_File->Tell();
  result const error = GetLastResult();
  if (IsFailed(error))
    MYTHROW(Writer::PosException, (m_FileName, m_Op, error, pos));
  return pos;
#else
  int64_t const pos = ftell64(m_File);
  if (pos == INVALID_POS)
    MYTHROW(Writer::PosException, (GetErrorProlog(), pos));

  ASSERT_GREATER_OR_EQUAL(pos, 0, ());
  return static_cast<uint64_t>(pos);
#endif
}

void FileData::Seek(uint64_t pos)
{
  ASSERT_NOT_EQUAL(m_Op, OP_APPEND, (m_FileName, m_Op, pos));
#ifdef OMIM_OS_TIZEN
  result const error = m_File->Seek(Tizen::Io::FILESEEKPOSITION_BEGIN, pos);
  if (IsFailed(error))
    MYTHROW(Writer::SeekException, (m_FileName, m_Op, error, pos));
#else
  if (fseek64(m_File, pos, SEEK_SET))
    MYTHROW(Writer::SeekException, (GetErrorProlog(), pos));
#endif
}

void FileData::Write(void const * p, size_t size)
{
#ifdef OMIM_OS_TIZEN
  result const error = m_File->Write(p, size);
  if (IsFailed(error))
    MYTHROW(Writer::WriteException, (m_FileName, m_Op, error, size));
#else
  size_t const bytesWritten = fwrite(p, 1, size, m_File);
  if (bytesWritten != size || ferror(m_File))
    MYTHROW(Writer::WriteException, (GetErrorProlog(), bytesWritten, size));
#endif
}

void FileData::Flush()
{
#ifdef OMIM_OS_TIZEN
  result const error = m_File->Flush();
  if (IsFailed(error))
    MYTHROW(Writer::WriteException, (m_FileName, m_Op, error));
#else
  if (fflush(m_File))
    MYTHROW(Writer::WriteException, (GetErrorProlog()));
#endif
}

void FileData::Truncate(uint64_t sz)
{
#ifdef OMIM_OS_WINDOWS
  int const res = _chsize(fileno(m_File), sz);
#elif defined OMIM_OS_TIZEN
  result res = m_File->Truncate(sz);
#else
  int const res = ftruncate(fileno(m_File), sz);
#endif

  if (res)
    MYTHROW(Writer::WriteException, (GetErrorProlog(), sz));
}

bool GetFileSize(string const & fName, uint64_t & sz)
{
  try
  {
    typedef my::FileData fdata_t;
    fdata_t f(fName, fdata_t::OP_READ);
    sz = f.Size();
    return true;
  }
  catch (RootException const &)
  {
    // supress all exceptions here
    return false;
  }
}

namespace
{
bool CheckFileOperationResult(int res, string const & fName)
{
  if (!res)
    return true;
#if !defined(OMIM_OS_TIZEN)
  LOG(LWARNING, ("File operation error for file:", fName, "-", strerror(errno)));
#endif

  // additional check if file really was removed correctly
  uint64_t dummy;
  if (GetFileSize(fName, dummy))
  {
    LOG(LERROR, ("File exists but can't be deleted. Sharing violation?", fName));
  }

  return false;
}
}  // namespace

bool DeleteFileX(string const & fName)
{
  int res;

#ifdef OMIM_OS_TIZEN
  res = IsFailed(Tizen::Io::File::Remove(fName.c_str())) ? -1 : 0;
#else
  res = remove(fName.c_str());
#endif

  return CheckFileOperationResult(res, fName);
}

bool RenameFileX(string const & fOld, string const & fNew)
{
  int res;

#ifdef OMIM_OS_TIZEN
  res = IsFailed(Tizen::Io::File::Move(fOld.c_str(), fNew.c_str())) ? -1 : 0;
#else
  res = rename(fOld.c_str(), fNew.c_str());
#endif

  return CheckFileOperationResult(res, fOld);
}

bool CopyFileX(string const & fOld, string const & fNew)
{
  try
  {
    ifstream ifs(fOld.c_str());
    ofstream ofs(fNew.c_str());

    if (ifs.is_open() && ofs.is_open())
    {
      ofs << ifs.rdbuf();
      ofs.flush();

      if (ofs.fail())
      {
        LOG(LWARNING, ("Bad or Fail bit is set while writing file:", fNew));
        return false;
      }

      return true;
    }
    else
      LOG(LERROR, ("Can't open files:", fOld, fNew));
  }
  catch (exception const & ex)
  {
    LOG(LERROR, ("Copy file error:", ex.what()));
  }

  return false;
}

bool IsEqualFiles(string const & firstFile, string const & secondFile)
{
  my::FileData first(firstFile, my::FileData::OP_READ);
  my::FileData second(secondFile, my::FileData::OP_READ);
  if (first.Size() != second.Size())
    return false;

  size_t const bufSize = READ_FILE_BUFFER_SIZE;
  vector<char> buf1, buf2;
  buf1.resize(bufSize);
  buf2.resize(bufSize);
  size_t const fileSize = first.Size();
  size_t currSize = 0;

  while (currSize < fileSize)
  {
    size_t const toRead = min(bufSize, fileSize - currSize);

    first.Read(currSize, &buf1[0], toRead);
    second.Read(currSize, &buf2[0], toRead);

    if (buf1 != buf2)
      return false;

    currSize += toRead;
  }

  return true;
}

}
