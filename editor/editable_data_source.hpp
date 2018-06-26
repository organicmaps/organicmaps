#pragma once

#include "editor/editable_feature_source.hpp"

#include "indexer/data_source.hpp"

class EditableDataSource : public DataSource
{
public:
  EditableDataSource() : DataSource(EditableFeatureSourceFactory::Get()) {}
};

class EditableFeaturesLoaderGuard : public DataSource::FeaturesLoaderGuard
{
public:
  EditableFeaturesLoaderGuard(DataSource const & dataSource, DataSource::MwmId const & id)
    : DataSource::FeaturesLoaderGuard(dataSource, id, EditableFeatureSourceFactory::Get())
  {
  }
};
