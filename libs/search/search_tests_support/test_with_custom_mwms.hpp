#pragma once

#include "editor/editable_data_source.hpp"
#include "editor/editor_tests_support/helpers.hpp"

#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "search/editor_delegate.hpp"

#include "indexer/feature.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>

namespace search
{
namespace tests_support
{
class TestWithCustomMwms : public generator::tests_support::TestWithCustomMwms
{
public:
  TestWithCustomMwms() { editor::tests_support::SetUpEditorForTesting(std::make_unique<EditorDelegate>(m_dataSource)); }

  ~TestWithCustomMwms() override { editor::tests_support::TearDownEditorForTesting(); }

  template <typename EditorFn>
  void EditFeature(FeatureID const & id, EditorFn && fn)
  {
    FeaturesLoaderGuard loader(m_dataSource, id.m_mwmId);
    auto ft = loader.GetFeatureByIndex(id.m_index);
    CHECK(ft, ());
    editor::tests_support::EditFeature(*ft, std::forward<EditorFn>(fn));
  }
};
}  // namespace tests_support
}  // namespace search
