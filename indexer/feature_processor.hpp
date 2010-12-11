#pragma once

#include "feature.hpp"

#include "../coding/varint.hpp"
#include "../coding/file_reader.hpp"

#include "../std/vector.hpp"

namespace feature
{
  template <class TSource, class TFeature>
  void ReadFromSource(TSource & src, TFeature & ft)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    vector<char> buffer(sz);
    src.Read(&buffer[0], sz);
    ft.Deserialize(buffer);
  }

  /// @return total header size, which should be skipped for data read, or 0 if error
  inline uint64_t ReadDatHeaderSize(Reader const & reader)
  {
    uint64_t const headerSize = ReadPrimitiveFromPos<uint64_t>(reader, 0);
    return headerSize + sizeof(uint64_t);
  }

  template <class ToDo>
  void ForEachFromDat(string const & fName, ToDo & toDo)
  {
    typedef ReaderSource<FileReader> source_t;

    FileReader reader(fName);
    source_t src(reader);

    // skip xml header
    uint64_t currPos = ReadDatHeaderSize(reader);
    src.Skip(currPos);
    uint64_t const fSize = reader.Size();
    // read features one by one
    while (currPos < fSize)
    {
      FeatureGeom ft;
      ReadFromSource(src, ft);
      toDo(ft, currPos);
      currPos = src.Pos();
    }
  }
}
