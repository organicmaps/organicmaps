#pragma once

#include "editor/editable_data_source.hpp"
#include "editor/editor_tests_support/helpers.hpp"

#include "indexer/indexer_tests_support/test_with_custom_mwms.hpp"

#include "search/editor_delegate.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include <utility>

namespace search
{
namespace tests_support
{
class TestWithCustomMwms : public indexer::tests_support::TestWithCustomMwms
{
public:
  TestWithCustomMwms()
  {
    editor::tests_support::SetUpEditorForTesting(my::make_unique<EditorDelegate>(m_index));
  }

  ~TestWithCustomMwms() override { editor::tests_support::TearDownEditorForTesting(); }

  template <typename EditorFn>
  void EditFeature(FeatureID const & id, EditorFn && fn)
  {
    EditableDataSource::FeaturesLoaderGuard loader(m_index, id.m_mwmId);
    FeatureType ft;
    CHECK(loader.GetFeatureByIndex(id.m_index, ft), ());
    editor::tests_support::EditFeature(ft, std::forward<EditorFn>(fn));
  }
};
}  // namespace tests_support
}  // namespace search
