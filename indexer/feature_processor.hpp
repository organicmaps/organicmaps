#pragma once

#include "indexer/features_vector.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_container.hpp"

#include <memory>
#include <string>
#include <utility>

namespace feature
{
template <class ToDo>
void ForEachFromDat(ModelReaderPtr reader, ToDo && toDo)
{
  FeaturesVectorTest features((FilesContainerR(reader)));
  features.GetVector().ForEach(std::forward<ToDo>(toDo));
}

template <class ToDo>
void ForEachFromDat(std::string const & fPath, ToDo && toDo)
{
  ForEachFromDat(std::make_unique<FileReader>(fPath), std::forward<ToDo>(toDo));
}
}  // namespace feature
