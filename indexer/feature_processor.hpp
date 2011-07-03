#pragma once

#include "features_vector.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"

#include "../std/bind.hpp"


namespace feature
{
  template <class ToDo>
  void ForEachFromDat(ModelReaderPtr reader, ToDo & toDo)
  {
    FilesContainerR container(reader);
    FeaturesVector featureSource(container);
    featureSource.ForEachOffset(bind<void>(ref(toDo), _1, _2));
  }

  template <class ToDo>
  void ForEachFromDat(string const & fPath, ToDo & toDo)
  {
    ForEachFromDat(new FileReader(fPath), toDo);
  }

   /// Read feature from feature source.
  template <class TSource>
  void ReadFromSourceRowFormat(TSource & src, FeatureBuilder1 & f)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    typename FeatureBuilder1::buffer_t buffer(sz);
    src.Read(&buffer[0], sz);
    f.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class ToDo>
  void ForEachFromDatRawFormat(string const & fName, ToDo & toDo)
  {
    FileReader reader(fName);
    ReaderSource<FileReader> src(reader);

    uint64_t currPos = 0;
    uint64_t const fSize = reader.Size();

    // read features one by one
    while (currPos < fSize)
    {
      FeatureBuilder1 f;
      ReadFromSourceRowFormat(src, f);
      toDo(f, currPos);
      currPos = src.Pos();
    }
  }
}
