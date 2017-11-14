#pragma once

#include "indexer/indexer_tests_support/helpers.hpp"
#include "indexer/indexer_tests_support/test_with_custom_mwms.hpp"

#include "search/editor_delegate.hpp"

#include "indexer/feature.hpp"
#include "indexer/index.hpp"

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
    indexer::tests_support::SetUpEditorForTesting(my::make_unique<EditorDelegate>(m_index));
  }

  ~TestWithCustomMwms() override = default;

  template <typename EditorFn>
  void EditFeature(FeatureID const & id, EditorFn && fn)
  {
    Index::FeaturesLoaderGuard loader(m_index, id.m_mwmId);
    FeatureType ft;
    CHECK(loader.GetFeatureByIndex(id.m_index, ft), ());
    indexer::tests_support::EditFeature(ft, std::forward<EditorFn>(fn));
  }
};
}  // namespace tests_support
}  // namespace search
