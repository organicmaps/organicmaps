#pragma once

#include "generator/generate_info.hpp"
#include "generator/osm_element.hpp"

#include "std/function.hpp"
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

  unique_ptr<istream, Deleter> m_file;

public:
  SourceReader();
  explicit SourceReader(string const & filename);
  explicit SourceReader(istringstream & stream);

  uint64_t Read(char * buffer, uint64_t bufferSize);
};



bool GenerateFeatures(feature::GenerateInfo & info);
bool GenerateIntermediateData(feature::GenerateInfo & info);

void BuildFeaturesFromO5M(SourceReader & stream, function<void(XMLElement *)> processor);
void BuildFeaturesFromXML(SourceReader & stream, function<void(XMLElement *)> processor);

