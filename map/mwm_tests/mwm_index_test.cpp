#include "testing/testing.hpp"

#include "map/features_fetcher.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/scales.hpp"

#include <string>
#include <vector>

using namespace std;

namespace
{
class CheckNonEmptyGeometry
{
public:
  vector<FeatureID> m_ids;

  void operator()(FeatureID const & id) { m_ids.push_back(id); }

  void operator()(FeatureType & ft)
  {
    bool res = false;
    ft.ForEachPoint([&res](m2::PointD const &) { res = true; }, m_scale);
    ft.ForEachTriangle([&res](m2::PointD const &, m2::PointD const &, m2::PointD const &) { res = true; }, m_scale);

    TEST(res, (ft.DebugString(), "Scale =", m_scale));
  }

  void SetScale(int scale)
  {
    m_ids.clear();
    m_scale = scale;
  }

private:
  int m_scale;
};

bool RunTest(string const & countryFileName, int lowS, int highS)
{
  FeaturesFetcher src;
  auto p = src.RegisterMap(platform::LocalCountryFile::MakeForTesting(countryFileName));
  if (p.second != MwmSet::RegResult::Success)
    return false;

  MwmSet::MwmId const & id = p.first;
  ASSERT(id.IsAlive(), ());

  version::Format const version = id.GetInfo()->m_version.GetFormat();
  if (version == version::Format::unknownFormat)
    return false;

  CheckNonEmptyGeometry doCheck;
  for (int scale = lowS; scale <= highS; ++scale)
  {
    doCheck.SetScale(scale);
    src.ForEachFeatureID(mercator::Bounds::FullRect(), doCheck, scale);
    src.ReadFeatures(doCheck, doCheck.m_ids);
  }

  return true;
}
}  // namespace

UNIT_TEST(ForEachFeatureID_Test)
{
  classificator::Load();

  TEST(RunTest("World", 0, scales::GetUpperWorldScale()), ());
  TEST(RunTest("WorldCoasts", 0, scales::GetUpperWorldScale()), ());
  // TEST(RunTest("Belarus", scales::GetUpperWorldScale() + 1, scales::GetUpperStyleScale()), ());
  TEST(RunTest("minsk-pass", scales::GetUpperWorldScale() + 1, scales::GetUpperStyleScale()), ());
}
