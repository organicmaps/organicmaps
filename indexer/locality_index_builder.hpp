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
#include <string>
#include <vector>

namespace covering
{
template <class TObjectsVector, class TWriter>
void BuildLocalityIndex(TObjectsVector const & objects, TWriter & writer,
                        string const & tmpFilePrefix)
{
  string const cellsToValueFile = tmpFilePrefix + CELL2LOCALITY_SORTED_EXT + ".all";
  MY_SCOPE_GUARD(cellsToValueFileGuard, bind(&FileWriter::DeleteFileX, cellsToValueFile));
  {
    FileWriter cellsToValueWriter(cellsToValueFile);

    WriterFunctor<FileWriter> out(cellsToValueWriter);
    FileSorter<CellValuePair<uint64_t>, WriterFunctor<FileWriter>> sorter(
        1024 * 1024 /* bufferBytes */, tmpFilePrefix + CELL2LOCALITY_TMP_EXT, out);
    objects.ForEach([&sorter](indexer::LocalityObject const & o) {
      // @todo(t.yan): adjust cellPenaltyArea for whole world locality index.
      std::vector<int64_t> const cells = covering::CoverLocality(
          o, GetCodingDepth<LocalityCellId::DEPTH_LEVELS>(scales::GetUpperScale()),
          250 /* cellPenaltyArea */);
      for (auto const & cell : cells)
        sorter.Add(CellValuePair<uint64_t>(cell, o.GetStoredId()));
    });
    sorter.SortAndFinish();
  }

  FileReader reader(cellsToValueFile);
  DDVector<CellValuePair<uint64_t>, FileReader, uint64_t> cellsToValue(reader);

  {
    BuildIntervalIndex(cellsToValue.begin(), cellsToValue.end(), writer,
                       LocalityCellId::DEPTH_LEVELS * 2 + 1);
  }
}
}  // namespace covering

namespace indexer
{
bool BuildLocalityIndexFromDataFile(std::string const & dataFile, std::string const & tmpFile);
}  // namespace indexer
