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
}
