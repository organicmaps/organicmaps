#pragma once

#include "indexer/features_vector.hpp"

#include "coding/file_reader.hpp"
#include "coding/files_container.hpp"

#include <memory>
#include <string>
#include <utility>

namespace feature
{
template <class ToDo>
void ForEachFeature(ModelReaderPtr const & reader, ToDo && toDo)
{
  FeaturesVectorTest features((FilesContainerR(reader)));
  features.GetVector().ForEach(std::forward<ToDo>(toDo));
}

template <class ToDo>
void ForEachFeature(std::string const & fPath, ToDo && toDo)
{
  ForEachFeature(std::make_unique<FileReader>(fPath), std::forward<ToDo>(toDo));
}
}  // namespace feature
