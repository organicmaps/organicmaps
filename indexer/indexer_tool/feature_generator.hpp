#pragma once

#include "../../indexer/osm_decl.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../coding/file_container.hpp"

#include "../../std/vector.hpp"
#include "../../std/string.hpp"

class FeatureBuilder1;

namespace feature
{
  struct GenerateInfo
  {
    GenerateInfo()
      : maxScaleForWorldFeatures(-1), splitByPolygons(false),
        simplifyCountriesLevel(-1), mergeCoastlines(false) {}
    string tmpDir, datFilePrefix, datFileSuffix;
    /// If not -1, world will be split by buckets with specified level
    int cellBucketingLevel;
    vector<string> bucketNames;
    /// Features with scale level [0..maxScaleForWorldFeatures] will be
    /// included into separate world data file
    /// @note if -1, world file will not be created
    int maxScaleForWorldFeatures;
    bool splitByPolygons;
    int simplifyCountriesLevel;
    bool mergeCoastlines;
  };

  bool GenerateFeatures(GenerateInfo & info, bool lightNodes);
  //bool GenerateCoastlines(GenerateInfo & info, bool lightNodes);

  // Writes features to dat file.
  class FeaturesCollector
  {
  protected:
    FileWriter m_datFile;

    m2::RectD m_bounds;

  protected:
    void Init();

    static uint32_t GetFileSize(FileWriter const & f);

    void WriteHeader();

    void WriteFeatureBase(vector<char> const & bytes, FeatureBuilder1 const & fb);

  public:
    // Stores prefix and suffix of a dat file name.
    typedef pair<string, string> InitDataType;

    FeaturesCollector(string const & fName);
    FeaturesCollector(string const & bucket, InitDataType const & prefix);
    ~FeaturesCollector();

    void operator() (FeatureBuilder1 const & f);
  };
}
