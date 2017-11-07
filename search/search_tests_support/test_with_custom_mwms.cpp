#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "search/editor_delegate.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/stl_add.hpp"

namespace search
{
namespace tests_support
{
TestWithCustomMwms::TestWithCustomMwms()
{
  indexer::tests_support::SetUpEditorForTesting(my::make_unique<EditorDelegate>(m_index));
}

TestWithCustomMwms::~TestWithCustomMwms()
{
  for (auto const & file : m_files)
    Cleanup(file);
}

// static
void TestWithCustomMwms::Cleanup(platform::LocalCountryFile const & file)
{
  platform::CountryIndexes::DeleteFromDisk(file);
  file.DeleteFromDisk(MapOptions::Map);
}
}  // namespace tests_support
}  // namespace search
