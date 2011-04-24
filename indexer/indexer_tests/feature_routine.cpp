#include "../../base/SRC_FIRST.hpp"

#include "feature_routine.hpp"

#include "../../defines.hpp"    // just for file extensions

#include "../../indexer/geometry_serialization.hpp"

#include "../../coding/file_writer.hpp"


namespace
{
  class feature_source_initializer
  {
    string m_name;
    FeatureType::read_source_t * m_source;

  public:
    feature_source_initializer(string const & fName)
      : m_name(fName), m_source(0)
    {
    }

    FeatureType::read_source_t & get_source(vector<char> & buffer)
    {
      delete m_source;
      m_source = new FeatureType::read_source_t(FilesContainerR(m_name));
      m_source->m_data.swap(buffer);
      return *m_source;
    }

    ~feature_source_initializer()
    {
      delete m_source;
      FileWriter::DeleteFileX(m_name);
    }
  };
}

void FeatureBuilder2Feature(FeatureBuilder2 & fb, FeatureType & f)
{
  string const datFile = "indexer_tests_tmp.dat";

  FeatureBuilder2::buffers_holder_t buffers;
  buffers.m_ptsOffset.push_back(0);
  buffers.m_trgOffset.push_back(0);
  buffers.m_ptsMask = 1;
  fb.Serialize(buffers, serial::CodingParams());

  {
    FilesContainerW writer(datFile);

    {
      FileWriter geom = writer.GetWriter(string(GEOMETRY_FILE_TAG) + '0');
      serial::SaveOuterPath(fb.GetGeometry(), serial::CodingParams(), geom);
    }

    //{
    //  FileWriter trg = writer.GetWriter(string(TRIANGLE_FILE_TAG) + '0');
    //  feature::SaveTriangles(fb.GetTriangles(), trg);
    //}

    writer.Finish();
  }

  static feature_source_initializer staticInstance(datFile);
  f.Deserialize(staticInstance.get_source(buffers.m_buffer));
}

void Feature2FeatureBuilder(FeatureType const & f, FeatureBuilder2 & fb)
{
  f.InitFeatureBuilder(fb);
}
