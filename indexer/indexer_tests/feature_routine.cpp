#include "../../base/SRC_FIRST.hpp"

#include "feature_routine.hpp"

#include "../../coding/file_writer.hpp"


void WriteToFile(string const & fName, vector<char> const & buffer)
{
  FileWriter writer(fName);
  if (!buffer.empty())
    writer.Write(&buffer[0], buffer.size());
}

void FeatureBuilder2Feature(FeatureBuilderGeomRef const & fb, FeatureGeomRef & f)
{
  string const datFile = "indexer_tests_tmp.dat";

  FeatureBuilderGeomRef::buffers_holder_t buffers;
  buffers.m_lineOffset = buffers.m_trgOffset = 0;
  fb.Serialize(buffers);

  WriteToFile(datFile + ".geom", buffers.m_buffers[1]);
  WriteToFile(datFile + ".trg", buffers.m_buffers[2]);

  static FeatureGeomRef::read_source_t g_source(FileReader(datFile + ".geom"), FileReader(datFile + ".trg"));
  g_source.m_data.swap(buffers.m_buffers[0]);
  f.Deserialize(g_source);

  FileWriter::DeleteFile(datFile + ".geom");
  FileWriter::DeleteFile(datFile + ".trg");
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
