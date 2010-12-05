#pragma once
#include "file64_api.hpp"
#include "../reader.hpp" // For Reader exceptions.
#include "../writer.hpp" // For Writer exceptions.
#include "../../base/base.hpp"
#include "../../base/exception.hpp"
#include "../../base/logging.hpp"
#include "../../std/string.hpp"

#ifdef OMIM_OS_BADA
  #include <FIoFile.h>
#endif

class FileData
{
public:
  enum Op { OP_READ = 0, OP_WRITE_TRUNCATE = 1, OP_WRITE_EXISTING = 2, OP_APPEND};

  FileData(string const & fileName, Op op) : m_FileName(fileName), m_Op(op)
  {
    char const * const modes [] = {"rb", "wb", "r+b", "ab"};
#ifdef OMIM_OS_BADA
    result error = m_File.Construct(fileName.c_str(), modes[op]);
    if (error == E_SUCCESS)
      return;
#else
    m_File = fopen(fileName.c_str(), modes[op]);
    int error = m_File ? ferror(m_File) : 0;
    if (m_File && !error)
      return;
    if (op == OP_WRITE_EXISTING)
    {
      // Special case, since "r+b" fails if file doesn't exist.
      if (m_File)
        fclose(m_File);
      m_File = fopen(fileName.c_str(), "wb");
      error = m_File ? ferror(m_File) : 0;
    }
    if (m_File && !error)
      return;
#endif
    // if we're here - something bad is happened
    if (m_Op)
      MYTHROW(Writer::OpenException, (fileName, error));
    else
      MYTHROW(Reader::OpenException, (fileName, error));
  }

  ~FileData()
  {
#ifndef OMIM_OS_BADA
    if (int error = fclose(m_File))
    {
      LOG(LWARNING, ("Error closing file", m_FileName, m_Op, error));
    }
#endif
  }

  uint64_t Size() const
  {
#ifdef OMIM_OS_BADA
    Osp::Io::FileAttributes attr;
    result error = Osp::Io::File::GetAttributes(m_FileName.c_str(), attr);
    if (IsFailed(error))
      MYTHROW(Reader::OpenException, (m_FileName, m_Op, error));
    return attr.GetFileSize();
#else
    uint64_t const pos = Pos();
    fseek64(m_File, 0, SEEK_END);
    if (int error = ferror(m_File))
      MYTHROW(Reader::OpenException, (m_FileName, m_Op, error));
    uint64_t size = ftell64(m_File);
    if (int error = ferror(m_File))
      MYTHROW(Reader::OpenException, (m_FileName, m_Op, error));
    fseek64(m_File, pos, SEEK_SET);
    if (int error = ferror(m_File))
      MYTHROW(Writer::SeekException, (m_FileName, m_Op, error, pos));
    return size;
#endif
  }

  void Read(uint64_t pos, void * p, size_t size)
  {
#ifdef OMIM_OS_BADA
    result error = m_File.Seek(Osp::Io::FILESEEKPOSITION_BEGIN, pos);
    if (IsFailed(error))
      MYTHROW(Reader::ReadException, (error, pos));
    int bytesRead = m_File.Read(p, size);
    error = GetLastResult();
    if (static_cast<size_t>(bytesRead) != size || IsFailed(error))
      MYTHROW(Reader::ReadException, (m_FileName, m_Op, error, bytesRead, pos, size));
#else
    fseek64(m_File, pos, SEEK_SET);
    if (int error = ferror(m_File))
        MYTHROW(Reader::ReadException, (error, pos));
    size_t bytesRead = fread(p, 1, size, m_File);
    int error = ferror(m_File);
    if (bytesRead != size || error)
      MYTHROW(Reader::ReadException, (m_FileName, m_Op, error, bytesRead, pos, size));
#endif
  }

  uint64_t Pos() const
  {
#ifdef OMIM_OS_BADA
    int pos = m_File.Tell();
    result error = GetLastResult();
    if (IsFailed(error))
      MYTHROW(Writer::PosException, (m_FileName, m_Op, error, pos));
    return pos;
#else
    uint64_t result = ftell64(m_File);
    if (int error = ferror(m_File))
      MYTHROW(Writer::PosException, (m_FileName, m_Op, error, result));
    return result;
#endif
  }

  void Seek(uint64_t pos)
  {
    ASSERT_NOT_EQUAL(m_Op, OP_APPEND, (m_FileName, m_Op, pos));
#ifdef OMIM_OS_BADA
    result error = m_File.Seek(Osp::Io::FILESEEKPOSITION_BEGIN, pos);
    if (IsFailed(error))
      MYTHROW(Writer::SeekException, (m_FileName, m_Op, error, pos));
#else
    fseek64(m_File, pos, SEEK_SET);
    if (int error = ferror(m_File))
      MYTHROW(Writer::SeekException, (m_FileName, m_Op, error, pos));
#endif
  }

  void Write(void const * p, size_t size)
  {
#ifdef OMIM_OS_BADA
    result error = m_File.Write(p, size);
    if (IsFailed(error))
      MYTHROW(Writer::WriteException, (m_FileName, m_Op, error, size));
#else
    size_t bytesWritten = fwrite(p, 1, size, m_File);
    int error = ferror(m_File);
    if (bytesWritten != size || error)
      MYTHROW(Writer::WriteException, (m_FileName, m_Op, error, bytesWritten, size));
#endif
  }

  void Flush()
  {
#ifdef OMIM_OS_BADA
    result error = m_File.Flush();
    if (IsFailed(error))
      MYTHROW(Writer::WriteException, (m_FileName, m_Op, error));
#else
    fflush(m_File);
    if (int error = ferror(m_File))
      MYTHROW(Writer::WriteException, (m_FileName, m_Op, error));
#endif
  }

  string GetName() const { return m_FileName; }

private:
  uint64_t m_LastAccessPos;
#ifdef OMIM_OS_BADA
  Osp::Io::File m_File;
#else
  FILE * m_File;
#endif
  string m_FileName;
  Op m_Op;
};
