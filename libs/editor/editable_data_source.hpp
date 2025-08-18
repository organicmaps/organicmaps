#pragma once

#include "editor/editable_feature_source.hpp"

#include "indexer/data_source.hpp"

#include <memory>

class EditableDataSource : public DataSource
{
public:
  EditableDataSource() : DataSource(std::make_unique<EditableFeatureSourceFactory>()) {}
};
