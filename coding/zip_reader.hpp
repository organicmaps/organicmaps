#pragma once

#include "file_reader.hpp"

#include "../base/exception.hpp"

class ZipFileReader : public FileReader
{
  typedef FileReader base_type;

public:
  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file);
};
