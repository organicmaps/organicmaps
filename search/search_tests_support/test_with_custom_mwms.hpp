#include "indexer/indexer_tests_support/test_with_custom_mwms.hpp"

#include "search/editor_delegate.hpp"

#include "base/stl_add.hpp"

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
};
}  // namespace tests_support
}  // namespace search
