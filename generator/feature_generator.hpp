#pragma once

#include "generate_info.hpp"
#include "osm_decl.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

class FeatureBuilder1;

namespace feature
{
  bool GenerateFeatures(GenerateInfo & info, bool lightNodes);

  // Writes features to dat file.
  class FeaturesCollector
  {
  protected:
    FileWriter m_datFile;

    m2::RectD m_bounds;

  protected:
    static uint32_t GetFileSize(FileWriter const & f);

    void WriteFeatureBase(vector<char> const & bytes, FeatureBuilder1 const & fb);

  public:
    // Stores prefix and suffix of a dat file name.
    typedef pair<string, string> InitDataType;

    FeaturesCollector(string const & fName);
    FeaturesCollector(string const & bucket, InitDataType const & prefix);

    void operator() (FeatureBuilder1 const & f);
  };
}
