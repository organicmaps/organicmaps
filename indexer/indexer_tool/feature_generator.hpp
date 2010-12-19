
#pragma once
#include "../../indexer/osm_decl.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../coding/file_writer.hpp"

#include "../../std/vector.hpp"
#include "../../std/string.hpp"

class FeatureBuilderGeom;
class FeatureBuilderGeomRef;

namespace feature
{
  struct GenerateInfo
  {
    GenerateInfo() : m_maxScaleForWorldFeatures(-1) {}
    string dir, datFilePrefix, datFileSuffix;
    int cellBucketingLevel;
    vector<string> bucketNames;
    /// Features with scale level [0..maxScaleForWorldFeatures] will be
    /// included into separate world data file
    /// @note if -1, world file will not be created
    int m_maxScaleForWorldFeatures;
  };

  bool GenerateFeatures(GenerateInfo & info, bool lightNodes);
  bool GenerateCoastlines(GenerateInfo & info, bool lightNodes);

  // Writes features to dat file.
  class FeaturesCollector
  {
    FileWriter m_datFile, m_geoFile, m_trgFile;
    m2::RectD m_bounds;

    void Init();
    void FilePreCondition(FileWriter const & f);

    static void WriteBuffer(FileWriter & f, vector<char> const & bytes);
    void WriteFeatureBase(vector<char> const & bytes, FeatureBuilderGeom const & fb);

  public:
    ~FeaturesCollector();

    // Stores prefix and suffix of a dat file name.
    typedef pair<string, string> InitDataType;

    explicit FeaturesCollector(string const & fName);
    FeaturesCollector(string const & bucket, InitDataType const & prefix);

    void operator() (FeatureBuilderGeom const & f);
    void operator() (FeatureBuilderGeomRef const & f);
  };
}
