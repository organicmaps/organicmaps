#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

namespace generator
{
namespace tests_support
{
TestWithCustomMwms::TestWithCustomMwms()
{
  m_version = base::GenerateYYMMDD(base::SecondsSinceEpoch());
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
  file.DeleteFromDisk(MapFileType::Map);
}

void TestWithCustomMwms::SetMwmVersion(uint32_t version) { m_version = version; }
}  // namespace tests_support
}  // namespace generator
