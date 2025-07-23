#include "coding/internal/file_data.hpp"

#include "coding/constants.hpp"
#include "coding/internal/file64_api.hpp"
#include "coding/reader.hpp"  // For Reader exceptions.
#include "coding/writer.hpp"  // For Writer exceptions.

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <exception>
#include <fstream>
#include <vector>

#ifdef OMIM_OS_WINDOWS
#include <io.h>
#else
#include <unistd.h>  // ftruncate
#endif

namespace base
{
using namespace std;

std::ostream & operator<<(std::ostream & stream, FileData::Op op)
{
  switch (op)
  {
  case FileData::Op::READ: stream << "READ"; break;
  case FileData::Op::WRITE_TRUNCATE: stream << "WRITE_TRUNCATE"; break;
  case FileData::Op::WRITE_EXISTING: stream << "WRITE_EXISTING"; break;
  case FileData::Op::APPEND: stream << "APPEND"; break;
  }
  return stream;
}

FileData::FileData(string const & fileName, Op op) : m_FileName(fileName), m_Op(op)
{
  char const * const modes[] = {"rb", "wb", "r+b", "ab"};

  m_File = fopen(fileName.c_str(), modes[static_cast<int>(op)]);
  if (m_File)
  {
#if defined(_MSC_VER)
    // Move file pointer to the end of the file to make it consistent with other platforms
    if (op == Op::APPEND)
      fseek64(m_File, 0, SEEK_END);
#endif
    return;
  }

  if (op == Op::WRITE_EXISTING)
  {
    // Special case, since "r+b" fails if file doesn't exist.
    m_File = fopen(fileName.c_str(), "wb");
    if (m_File)
      return;
  }

  // if we're here - something bad is happened
  if (m_Op != Op::READ)
    MYTHROW(Writer::OpenException, (GetErrorProlog()));
  else
    MYTHROW(Reader::OpenException, (GetErrorProlog()));
}

FileData::~FileData()
{
  if (m_File)
  {
    if (fclose(m_File))
      LOG(LWARNING, ("Error closing file", GetErrorProlog()));
  }
}

string FileData::GetErrorProlog() const
{
  std::ostringstream stream;
  stream << m_FileName << "; " << m_Op << "; " << strerror(errno);
  return stream.str();
}

static int64_t constexpr INVALID_POS = -1;

uint64_t FileData::Size() const
{
  int64_t const pos = ftell64(m_File);
  if (pos == INVALID_POS)
    MYTHROW(Reader::SizeException, (GetErrorProlog(), pos));

  if (fseek64(m_File, 0, SEEK_END))
    MYTHROW(Reader::SizeException, (GetErrorProlog()));

  int64_t const size = ftell64(m_File);
  if (size == INVALID_POS)
    MYTHROW(Reader::SizeException, (GetErrorProlog(), size));

  if (fseek64(m_File, static_cast<off_t>(pos), SEEK_SET))
    MYTHROW(Reader::SizeException, (GetErrorProlog(), pos));

  ASSERT_GREATER_OR_EQUAL(size, 0, ());
  return static_cast<uint64_t>(size);
}

void FileData::Read(uint64_t pos, void * p, size_t size)
{
  if (fseek64(m_File, static_cast<off_t>(pos), SEEK_SET))
    MYTHROW(Reader::ReadException, (GetErrorProlog(), pos));

  size_t const bytesRead = fread(p, 1, size, m_File);
  if (bytesRead != size || ferror(m_File))
    MYTHROW(Reader::ReadException, (GetErrorProlog(), bytesRead, pos, size));
}

uint64_t FileData::Pos() const
{
  int64_t const pos = ftell64(m_File);
  if (pos == INVALID_POS)
    MYTHROW(Writer::PosException, (GetErrorProlog(), pos));

  ASSERT_GREATER_OR_EQUAL(pos, 0, ());
  return static_cast<uint64_t>(pos);
}

void FileData::Seek(uint64_t pos)
{
  ASSERT_NOT_EQUAL(m_Op, Op::APPEND, (m_FileName, m_Op, pos));
  if (fseek64(m_File, static_cast<off_t>(pos), SEEK_SET))
    MYTHROW(Writer::SeekException, (GetErrorProlog(), pos));
}

void FileData::Write(void const * p, size_t size)
{
  size_t const bytesWritten = fwrite(p, 1, size, m_File);
  if (bytesWritten != size || ferror(m_File))
    MYTHROW(Writer::WriteException, (GetErrorProlog(), bytesWritten, size));
}

void FileData::Flush()
{
  if (fflush(m_File))
    MYTHROW(Writer::WriteException, (GetErrorProlog()));
}

void FileData::Truncate(uint64_t sz)
{
#ifdef OMIM_OS_WINDOWS
  int const res = _chsize(fileno(m_File), sz);
#else
  int const res = ftruncate(fileno(m_File), static_cast<off_t>(sz));
#endif

  if (res)
    MYTHROW(Writer::WriteException, (GetErrorProlog(), sz));
}

bool GetFileSize(string const & fName, uint64_t & sz)
{
  try
  {
    typedef FileData fdata_t;
    fdata_t f(fName, fdata_t::Op::READ);
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

  LOG(LWARNING, ("File operation error for file:", fName, "-", strerror(errno)));

  // additional check if file really was removed correctly
  uint64_t dummy;
  if (GetFileSize(fName, dummy))
    LOG(LERROR, ("File exists but can't be deleted. Sharing violation?", fName));

  return false;
}

bool IsEOF(ifstream & fs)
{
  return fs.peek() == ifstream::traits_type::eof();
}

}  // namespace

bool DeleteFileX(string const & fName)
{
  int res = remove(fName.c_str());
  return CheckFileOperationResult(res, fName);
}

bool RenameFileX(string const & fOld, string const & fNew)
{
  int res = rename(fOld.c_str(), fNew.c_str());
  return CheckFileOperationResult(res, fOld);
}

bool MoveFileX(string const & fOld, string const & fNew)
{
  // Try to rename the file first.
  int res = rename(fOld.c_str(), fNew.c_str());
  if (res == 0)
    return true;

  // Otherwise perform the full move.
  if (!CopyFileX(fOld, fNew))
  {
    (void)DeleteFileX(fNew);
    return false;
  }
  (void)DeleteFileX(fOld);
  return true;
}

bool WriteToTempAndRenameToFile(string const & dest, function<bool(string const &)> const & write, string const & tmp)
{
  string const tmpFileName = tmp.empty() ? dest + ".tmp" + strings::to_string(this_thread::get_id()) : tmp;
  if (!write(tmpFileName))
  {
    LOG(LERROR, ("Can't write to", tmpFileName));
    DeleteFileX(tmpFileName);
    return false;
  }
  if (!RenameFileX(tmpFileName, dest))
  {
    LOG(LERROR, ("Can't rename file", tmpFileName, "to", dest));
    DeleteFileX(tmpFileName);
    return false;
  }
  return true;
}

void AppendFileToFile(string const & fromFilename, string const & toFilename)
{
  ifstream from;
  from.exceptions(fstream::failbit | fstream::badbit);
  from.open(fromFilename, ios::binary);

  ofstream to;
  to.exceptions(fstream::badbit);
  to.open(toFilename, ios::binary | ios::app);

  auto * buffer = from.rdbuf();
  if (!IsEOF(from))
    to << buffer;
}

bool CopyFileX(string const & fOld, string const & fNew)
{
  ifstream ifs;
  ofstream ofs;
  ifs.exceptions(ifstream::failbit | ifstream::badbit);
  ofs.exceptions(ifstream::failbit | ifstream::badbit);

  try
  {
    ifs.open(fOld.c_str());
    ofs.open(fNew.c_str());

    // If source file is empty - make empty dest file without any errors.
    if (IsEOF(ifs))
      return true;

    ofs << ifs.rdbuf();
    ofs.flush();
    return true;
  }
  catch (system_error const &)
  {
    LOG(LWARNING, ("Failed to copy file from", fOld, "to", fNew, ":", strerror(errno)));
  }
  catch (exception const &)
  {
    LOG(LERROR, ("Unknown error when coping files:", fOld, "to", fNew, strerror(errno)));
  }

  // Don't care about possible error here ..
  (void)DeleteFileX(fNew);
  return false;
}

bool IsEqualFiles(string const & firstFile, string const & secondFile)
{
  FileData first(firstFile, FileData::Op::READ);
  FileData second(secondFile, FileData::Op::READ);
  if (first.Size() != second.Size())
    return false;

  size_t constexpr bufSize = READ_FILE_BUFFER_SIZE;
  vector<char> buf1, buf2;
  buf1.resize(bufSize);
  buf2.resize(bufSize);
  size_t const fileSize = static_cast<size_t>(first.Size());
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

std::vector<uint8_t> ReadFile(std::string const & filePath)
{
  FileData file(filePath, FileData::Op::READ);
  uint64_t const sz = file.Size();
  std::vector<uint8_t> contents(sz);
  file.Read(0, contents.data(), sz);
  return contents;
}

}  // namespace base
