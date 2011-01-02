
#pragma once
#include "../../indexer/osm_decl.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../coding/file_container.hpp"

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
  protected:
    FileWriter m_datFile;

    m2::RectD m_bounds;

  protected:
    void Init();
    void FilePreCondition(FileWriter const & f);

    void WriteHeader();

    static void WriteBuffer(FileWriter & f, vector<char> const & bytes);
    void WriteFeatureBase(vector<char> const & bytes, FeatureBuilderGeom const & fb);

  public:
    // Stores prefix and suffix of a dat file name.
    typedef pair<string, string> InitDataType;

    FeaturesCollector(string const & fName);
    FeaturesCollector(string const & bucket, InitDataType const & prefix);
    ~FeaturesCollector();

    void operator() (FeatureBuilderGeom const & f);
  };

  class FeaturesCollectorRef : public FeaturesCollector
  {
    FilesContainerW m_writer;
    FileWriter m_geoFile, m_trgFile;

  public:
    explicit FeaturesCollectorRef(string const & fName);
    ~FeaturesCollectorRef();

    void operator() (FeatureBuilderGeomRef const & f);
  };
}
