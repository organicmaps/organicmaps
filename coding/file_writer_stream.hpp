#pragma once

#include "coding/streams.hpp"
#include "coding/file_writer.hpp"

class FileWriterStream : public stream::WriterStream<FileWriter>
{
  typedef stream::WriterStream<FileWriter> base_type;

  FileWriter m_file;

public:
  FileWriterStream(string const & fName)
    : base_type(m_file), m_file(fName) {}

  using base_type::operator <<;

  int64_t Pos() const { return m_file.Pos(); }
};
