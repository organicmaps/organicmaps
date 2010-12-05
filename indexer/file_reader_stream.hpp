#pragma once

#include "../coding/streams.hpp"
#include "../coding/file_reader.hpp"

/// @todo Remove and use ReadPrimitive() and other free functions.
class FileReaderStream : public stream::ReaderStream<ReaderSource<FileReader> >
{
  typedef stream::ReaderStream<ReaderSource<FileReader> > base_type;

  FileReader m_file;
  ReaderSource<FileReader> m_reader;

public:
  FileReaderStream(char const * fName)
    : base_type(m_reader), m_file(fName), m_reader(m_file)
  {
  }

  using base_type::operator >>;

  void Seek(uint64_t pos)
  {
    m_reader = m_file.SubReader(pos, m_file.Size() - pos);
  }
};
