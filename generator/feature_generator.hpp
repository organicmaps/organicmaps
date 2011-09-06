#pragma once

#include "../geometry/rect2d.hpp"

#include "../coding/file_writer.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"


class FeatureBuilder1;

namespace feature
{
  class GenerateInfo;

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
    FeaturesCollector(string const & fName);

    void operator() (FeatureBuilder1 const & f);
  };

  static const int g_coastsCellLevel = 6;
}
