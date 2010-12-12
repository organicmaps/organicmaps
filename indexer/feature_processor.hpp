#pragma once

#include "feature.hpp"
#include "data_header_reader.hpp"

#include "../coding/varint.hpp"
#include "../coding/file_reader.hpp"

#include "../std/vector.hpp"

namespace feature
{
  /// Read feature from feature source.
  template <class TSource, class TFeature>
  void ReadFromSource(TSource & src, TFeature & f)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    vector<char> buffer(sz);
    src.Read(&buffer[0], sz);
    f.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class TFeature, class ToDo>
  void ForEachFromDat(string const & fName, ToDo & toDo)
  {
    typedef ReaderSource<FileReader> source_t;

    FileReader reader(fName);
    source_t src(reader);

    // skip header
    uint64_t currPos = feature::GetSkipHeaderSize(reader);
    src.Skip(currPos);

    uint64_t const fSize = reader.Size();
    // read features one by one
    while (currPos < fSize)
    {
      TFeature f;
      ReadFromSource(src, f);
      toDo(f, currPos);
      currPos = src.Pos();
    }
  }
}
