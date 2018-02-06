#include "testing/testing.hpp"

#include "map/cloud.hpp"

#include "coding/serdes_json.hpp"
#include "coding/writer.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono;

namespace
{
bool AreEqualEntries(std::vector<Cloud::EntryPtr> const & entries1,
                     std::vector<Cloud::EntryPtr> const & entries2)
{
  if (entries1.size() != entries2.size())
    return false;

  for (size_t i = 0; i < entries1.size(); ++i)
  {
    if (*entries1[i] != *entries2[i])
      return false;
  }

  return true;
}
}  // namespace

UNIT_TEST(Cloud_SerDes)
{
  Cloud::Index index;
  index.m_entries.emplace_back(std::make_shared<Cloud::Entry>("bm1.kml", 100, false));
  index.m_entries.emplace_back(std::make_shared<Cloud::Entry>("bm2.kml", 50, true));
  auto const h = duration_cast<hours>(system_clock::now().time_since_epoch()).count();
  index.m_lastUpdateInHours = static_cast<uint64_t>(h);
  index.m_isOutdated = true;

  std::string data;
  {
    using Sink = MemWriter<string>;
    Sink sink(data);
    coding::SerializerJson<Sink> ser(sink);
    ser(index);
  }

  Cloud::Index indexDes;
  {
    coding::DeserializerJson des(data);
    des(indexDes);
  }

  TEST_EQUAL(index.m_isOutdated, indexDes.m_isOutdated, ());
  TEST_EQUAL(index.m_lastUpdateInHours, indexDes.m_lastUpdateInHours, ());
  TEST(AreEqualEntries(index.m_entries, indexDes.m_entries), ());
}
