#include "ugc/index_migration/utility.hpp"
#include "ugc/serdes.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std;

namespace
{
string const kBinExt = ".bin";
string const kMigrationDirName = "ugc_migration";

using MigrationTable = unordered_map<uint32_t, uint32_t>;
using MigrationTables = unordered_map<int64_t, MigrationTable>;

bool GetMigrationTable(int64_t tableVersion, MigrationTable & t)
{
  auto const fileName = my::JoinPath(kMigrationDirName, to_string(tableVersion) + kBinExt);
  try
  {
    auto reader = GetPlatform().GetReader(fileName);
    NonOwningReaderSource source(*reader);
    ugc::DeserializerV0<NonOwningReaderSource> des(source);
    des(t);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, (ex.what()));
    return false;
  }

  if (t.empty())
  {
    ASSERT(false, ());
    return false;
  }

  return true;
}

bool MigrateFromV0ToV1(ugc::UpdateIndexes & source)
{
  MigrationTables tables;
  for (auto & index : source)
  {
    auto const version = index.m_dataVersion;
    if (tables.find(version) == tables.end())
    {
      MigrationTable t;
      if (!GetMigrationTable(version, t))
        return false;

      tables.emplace(version, move(t));
    }

    auto & t = tables[version];
    index.m_type = t[index.m_type];
    index.m_matchingType = t[index.m_matchingType];
    index.m_synchronized = false;
    index.m_version = ugc::IndexVersion::Latest;
  }

  return true;
}
}  // namespace

namespace ugc
{
namespace migration
{
Result Migrate(UpdateIndexes & source)
{
  CHECK(!source.empty(), ());
  if (source.front().m_version == IndexVersion::Latest)
    return Result::UpToDate;

  auto const result = MigrateFromV0ToV1(source);
  return result ? Result::Success : Result::Failure;
}
}  // namespace migration
}  // namespace ugc
