#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/cell_value_pair.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/interval_index_builder.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "coding/dd_vector.hpp"
#include "coding/file_sort.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace covering
{
using CoverLocality =
    std::function<std::vector<int64_t>(indexer::LocalityObject const & o, int cellDepth)>;

template <class ObjectsVector, class Writer, int DEPTH_LEVELS>
void BuildLocalityIndex(ObjectsVector const & objects, Writer & writer,
                        CoverLocality const & coverLocality, string const & tmpFilePrefix)
{
  string const cellsToValueFile = tmpFilePrefix + CELL2LOCALITY_SORTED_EXT + ".all";
  MY_SCOPE_GUARD(cellsToValueFileGuard, bind(&FileWriter::DeleteFileX, cellsToValueFile));
  {
    FileWriter cellsToValueWriter(cellsToValueFile);

    WriterFunctor<FileWriter> out(cellsToValueWriter);
    FileSorter<CellValuePair<uint64_t>, WriterFunctor<FileWriter>> sorter(
        1024 * 1024 /* bufferBytes */, tmpFilePrefix + CELL2LOCALITY_TMP_EXT, out);
    objects.ForEach([&sorter, &coverLocality](indexer::LocalityObject const & o) {
      std::vector<int64_t> const cells =
          coverLocality(o, GetCodingDepth<DEPTH_LEVELS>(scales::GetUpperScale()));
      for (auto const & cell : cells)
        sorter.Add(CellValuePair<uint64_t>(cell, o.GetStoredId()));
    });
    sorter.SortAndFinish();
  }

  FileReader reader(cellsToValueFile);
  DDVector<CellValuePair<uint64_t>, FileReader, uint64_t> cellsToValue(reader);

  {
    BuildIntervalIndex(cellsToValue.begin(), cellsToValue.end(), writer, DEPTH_LEVELS * 2 + 1);
  }
}
}  // namespace covering

namespace indexer
{
// Builds indexer::GeoObjectsIndex for reverse geocoder with |kGeoObjectsDepthLevels| depth levels
// and saves it to |GEO_OBJECTS_INDEX_FILE_TAG| of |out|.
bool BuildGeoObjectsIndexFromDataFile(std::string const & dataFile, std::string const & out);

// Builds indexer::RegionsIndex for reverse geocoder with |kRegionsDepthLevels| depth levels and
// saves it to |REGIONS_INDEX_FILE_TAG| of |out|.
bool BuildRegionsIndexFromDataFile(std::string const & dataFile, std::string const & out);
}  // namespace indexer
