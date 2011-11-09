#pragma once

#include "../coding/streams.hpp"
#include "../coding/file_reader.hpp"


class ReaderPtrStream : public stream::ReaderStream<ReaderSource<ReaderPtr<Reader> > >
{
  typedef ReaderPtr<Reader> ptr_t;
  typedef ReaderSource<ptr_t> source_t;
  typedef stream::ReaderStream<source_t> base_type;

  source_t m_src;
public:
  ReaderPtrStream(Reader * p) : base_type(m_src), m_src(p) {}
  ReaderPtrStream(ptr_t const & p) : base_type(m_src), m_src(p) {}
};


class FileReaderStream : public stream::ReaderStream<ReaderSource<FileReader> >
{
  typedef stream::ReaderStream<ReaderSource<FileReader> > base_type;

  FileReader m_file;
  ReaderSource<FileReader> m_reader;

public:
  FileReaderStream(string const & fName)
    : base_type(m_reader), m_file(fName), m_reader(m_file)
  {
  }

  using base_type::operator >>;

  // It is neccesary for DataFileReader.
  void Seek(uint64_t pos)
  {
    m_reader = m_file.SubReader(pos, m_file.Size() - pos);
  }
};
