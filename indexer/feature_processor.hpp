#pragma once

#include "features_vector.hpp"

#include "../coding/file_container.hpp"

#include "../std/bind.hpp"


namespace feature
{
  template <class ToDo>
  void ForEachFromDat(string const & fName, ToDo & toDo)
  {
    FilesContainerR container(fName);
    FeaturesVector<FileReader> featureSource(container);
    featureSource.ForEachOffset(bind<void>(ref(toDo), _1, _2));
  }

   /// Read feature from feature source.
  template <class TSource>
  void ReadFromSource(TSource & src, FeatureGeom & f, typename FeatureGeom::read_source_t & buffer)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    buffer.m_data.resize(sz);
    src.Read(&buffer.m_data[0], sz);
    f.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class ToDo>
  void ForEachFromDatRawFormat(string const & fName, ToDo & toDo)
  {
    FileReader reader(fName);
    ReaderSource<FileReader> src(reader);

    // skip header
    uint64_t currPos = feature::GetSkipHeaderSize(reader);
    src.Skip(currPos);

    uint64_t const fSize = reader.Size();

    // read features one by one
    typename FeatureGeom::read_source_t buffer;
    while (currPos < fSize)
    {
      FeatureGeom f;
      ReadFromSource(src, f, buffer);
      toDo(f, currPos);
      currPos = src.Pos();
    }
  }
}
