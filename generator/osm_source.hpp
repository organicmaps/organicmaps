#pragma once

#include "generator/generate_info.hpp"

#include "std/iostream.hpp"
#include "std/unique_ptr.hpp"

class SourceReader
{
  struct Deleter
  {
    bool m_needDelete;
    Deleter(bool needDelete = true) : m_needDelete(needDelete) {}
    void operator()(istream * s) const
    {
      if (m_needDelete)
        delete s;
    }
  };

  string m_filename;
  unique_ptr<istream, Deleter> m_file;

public:
  SourceReader();
  SourceReader(string const & filename);
  SourceReader(istringstream & stream);

  uint64_t Read(char * buffer, uint64_t bufferSize);
};



bool GenerateFeatures(feature::GenerateInfo & info);
bool GenerateIntermediateData(feature::GenerateInfo & info);

class BaseOSMParser;

void BuildFeaturesFromO5M(SourceReader & stream, BaseOSMParser & parser);

