#include "../../base/SRC_FIRST.hpp"

#include "feature_routine.hpp"

#include "../../coding/file_writer.hpp"


void WriteToFile(string const & fName, vector<char> const & buffer)
{
  FileWriter writer(fName);
  if (!buffer.empty())
    writer.Write(&buffer[0], buffer.size());
}

string g_datFile = "indexer_tests_tmp.dat";
FeatureGeomRef::read_source_t g_source(g_datFile);

void FeatureBuilder2Feature(FeatureBuilderGeomRef const & fb, FeatureGeomRef & f)
{
  FeatureBuilderGeomRef::buffers_holder_t buffers;
  buffers.m_lineOffset = buffers.m_trgOffset = 0;
  fb.Serialize(buffers);

  WriteToFile(g_datFile + ".geom", buffers.m_buffers[1]);
  WriteToFile(g_datFile + ".trg", buffers.m_buffers[2]);

  g_source.m_data.swap(buffers.m_buffers[0]);
  f.Deserialize(g_source);
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
