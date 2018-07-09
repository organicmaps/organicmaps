#include "indexer/borders.hpp"

#include "coding/file_container.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/var_record_reader.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "defines.hpp"

using namespace std;

namespace
{
template <class Reader>
class BordersVector
{
public:
  BordersVector(Reader const & reader) : m_recordReader(reader, 256 /* expectedRecordSize */) {}

  template <class ToDo>
  void ForEach(ToDo && toDo) const
  {
    m_recordReader.ForEachRecord([&](uint32_t pos, char const * data, uint32_t /*size*/) {
      ArrayByteSource src(data);
      serial::GeometryCodingParams cp = {};

      auto readPoly = [&cp, &src](vector<m2::PointD> & poly) {
        m2::PointU base = cp.GetBasePoint();
        for (auto & point : poly)
        {
          base = coding::DecodePointDelta(src, base);
          point = PointUToPointD(base, cp.GetCoordBits());
        }
      };

      uint64_t id;
      ReadPrimitiveFromSource(src, id);
      size_t size;
      ReadPrimitiveFromSource(src, size);
      size_t outerSize;
      ReadPrimitiveFromSource(src, outerSize);
      vector<m2::PointD> outer(outerSize);
      readPoly(outer);

      vector<vector<m2::PointD>> inners(size);
      for (auto & inner : inners)
      {
        size_t innerSize;
        ReadPrimitiveFromSource(src, innerSize);
        inner = vector<m2::PointD>(innerSize);
        readPoly(inner);
      }
      toDo(id, outer, inners);
    });
  }

private:
  friend class BordersVectorReader;

  VarRecordReader<FilesContainerR::TReader, &VarRecordSizeReaderVarint> m_recordReader;

  DISALLOW_COPY(BordersVector);
};

class BordersVectorReader
{
public:
  explicit BordersVectorReader(string const & filePath)
    : m_cont(filePath), m_vector(m_cont.GetReader(BORDERS_FILE_TAG))
  {
  }

  BordersVector<ModelReaderPtr> const & GetVector() const { return m_vector; }

private:
  FilesContainerR m_cont;
  BordersVector<ModelReaderPtr> m_vector;

  DISALLOW_COPY(BordersVectorReader);
};
}  // namespace

namespace indexer
{
bool Borders::Border::IsPointInside(m2::PointD const & point) const
{
  if (!m_outer.Contains(point))
    return false;

  for (auto const & inner : m_inners)
  {
    if (inner.Contains(point))
      return false;
  }

  return true;
}

void Borders::Deserialize(string const & filename)
{
  BordersVectorReader reader(filename);
  auto const & records = reader.GetVector();
  DeserializeFromVec(records);
}
}  // namespace indexer
