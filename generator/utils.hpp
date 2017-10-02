#pragma once

#include "generator/gen_mwm_info.hpp"

#include "indexer/index.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"

namespace generator
{
void LoadIndex(Index & index);

template <typename ToDo>
bool ForEachOsmId2FeatureId(string const & path, ToDo && toDo)
{
  gen::OsmID2FeatureID mapping;
  try
  {
    FileReader reader(path);
    NonOwningReaderSource source(reader);
    mapping.Read(source);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LERROR, ("Exception while reading file:", path, ", message:", e.Msg()));
    return false;
  }

  mapping.ForEach([&](gen::OsmID2FeatureID::ValueT const & p) {
    toDo(p.first /* osm id */, p.second /* feature id */);
  });

  return true;
}
}  // namespace generator
