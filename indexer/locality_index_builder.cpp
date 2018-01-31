#include "indexer/locality_index_builder.hpp"

#include "indexer/locality_object.hpp"

#include "defines.hpp"

#include "coding/file_container.hpp"
#include "coding/var_record_reader.hpp"

#include "base/logging.hpp"

namespace indexer
{
namespace
{
template <class ReaderT>
class LocalityVector
{
  DISALLOW_COPY(LocalityVector);

public:
  LocalityVector(ReaderT const & reader) : m_recordReader(reader, 256 /* expectedRecordSize */) {}

  template <class ToDo>
  void ForEach(ToDo && toDo) const
  {
    m_recordReader.ForEachRecord([&](uint32_t pos, char const * data, uint32_t /*size*/) {
      LocalityObject o;
      o.Deserialize(data);
      toDo(o);
    });
  }

private:
  friend class LocalityVectorReader;

  VarRecordReader<FilesContainerR::TReader, &VarRecordSizeReaderVarint> m_recordReader;
};

// Test features vector (reader) that combines all the needed data for stand-alone work.
// Used in generator_tool and unit tests.
class LocalityVectorReader
{
  DISALLOW_COPY(LocalityVectorReader);

public:
  explicit LocalityVectorReader(string const & filePath)
    : m_cont(filePath), m_vector(m_cont.GetReader(LOCALITY_DATA_FILE_TAG))
  {
  }

  LocalityVector<ModelReaderPtr> const & GetVector() const { return m_vector; }

private:
  FilesContainerR m_cont;
  LocalityVector<ModelReaderPtr> m_vector;
};
}  // namespace

bool BuildLocalityIndexFromDataFile(string const & dataFile, string const & outFileName)
{
  try
  {
    string const idxFileName(outFileName + LOCALITY_INDEX_TMP_EXT);
    {
      LocalityVectorReader localities(dataFile);
      FileWriter writer(idxFileName);

      covering::BuildLocalityIndex(localities.GetVector(), writer, outFileName);
    }

    FilesContainerW(outFileName, FileWriter::OP_WRITE_TRUNCATE)
        .Write(idxFileName, LOCALITY_INDEX_FILE_TAG);
    FileWriter::DeleteFileX(idxFileName);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file:", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace indexer
