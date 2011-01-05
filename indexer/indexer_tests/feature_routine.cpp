#include "../../base/SRC_FIRST.hpp"

#include "feature_routine.hpp"

#include "../feature_impl.hpp"

#include "../../coding/file_writer.hpp"


namespace
{
  class feature_source_initializer
  {
    string m_name;
    FeatureGeomRef::read_source_t * m_source;

  public:
    feature_source_initializer(string const & fName)
      : m_name(fName), m_source(0)
    {
    }

    FeatureGeomRef::read_source_t & get_source(vector<char> & buffer)
    {
      delete m_source;
      m_source = new FeatureGeomRef::read_source_t(FilesContainerR(m_name));
      m_source->m_data.swap(buffer);
      return *m_source;
    }

    ~feature_source_initializer()
    {
      delete m_source;
      FileWriter::DeleteFile(m_name);
    }
  };
}

void FeatureBuilder2Feature(FeatureBuilderGeomRef const & fb, FeatureGeomRef & f)
{
  string const datFile = "indexer_tests_tmp.dat";

  FeatureBuilderGeomRef::buffers_holder_t buffers;
  buffers.m_lineOffset.push_back(0);
  buffers.m_trgOffset.push_back(0);
  buffers.m_mask = 1;
  fb.Serialize(buffers);

  {
    FilesContainerW writer(datFile);

    {
      FileWriter geom = writer.GetWriter(string(GEOMETRY_FILE_TAG) + '0');
      feature::SerializePoints(fb.GetGeometry(), geom);
    }

    {
      FileWriter trg = writer.GetWriter(string(TRIANGLE_FILE_TAG) + '0');
      feature::SerializeTriangles(fb.GetTriangles(), trg);
    }

    writer.Finish();
  }

  static feature_source_initializer staticInstance(datFile);
  f.Deserialize(staticInstance.get_source(buffers.m_buffer));
}

void Feature2FeatureBuilder(FeatureGeomRef const & f, FeatureBuilderGeomRef & fb)
{
  f.InitFeatureBuilder(fb);
}

void FeatureBuilder2Feature(FeatureBuilderGeom const & fb, FeatureGeom & f)
{
  FeatureBuilderGeom::buffers_holder_t buffers;
  fb.Serialize(buffers);

  FeatureGeom::read_source_t source;
  source.m_data.swap(buffers);
  f.Deserialize(source);
}

void Feature2FeatureBuilder(FeatureGeom const & f, FeatureBuilderGeom & fb)
{
  f.InitFeatureBuilder(fb);
}
