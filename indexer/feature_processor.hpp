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
  void ReadFromSource(TSource & src, TFeature & f, typename TFeature::read_source_t & buffer)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    buffer.m_data.resize(sz);
    src.Read(&buffer.m_data[0], sz);
    f.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class TFeature, class ToDo>
  void ForEachFromDat(string const & fName, ToDo & toDo)
  {
    typedef ReaderSource<FileReader> source_t;

    FileReader reader(fName);
    source_t src(reader);
    typename TFeature::read_source_t buffer(fName);

    // skip header
    uint64_t currPos = feature::GetSkipHeaderSize(reader);
    src.Skip(currPos);

    uint64_t const fSize = reader.Size();
    // read features one by one
    while (currPos < fSize)
    {
      TFeature f;
      ReadFromSource(src, f, buffer);
      toDo(f, currPos);
      currPos = src.Pos();
    }
  }
}
